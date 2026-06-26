#include "analyzer.hpp"

#include "parse_ctx.hpp"
#include "ast.hpp"
#include "frontend_error.hpp"
#include "module.hpp"
#include "module_manager.hpp"
#include "util/fmt.hpp"

using enum pars::TokenType;

void pars::Analyzer::visit(CallExpr *expr, VisitCtx ctx)
{
	expr->prototype = find_symbol<FnDecl>(expr->symbol, expr->token);

	expr->prototype->accept(this, {});

	if (!expr->prototype->signature.is_variadic &&
		(expr->arguments.size() < expr->prototype->signature.callable_arity ||
		expr->arguments.size() > expr->prototype->signature.parameters.size()))
	{
		throw FrontendError{expr->token, fmt::format("expected {} argument but got {}",
			expr->prototype->signature.callable_arity, expr->arguments.size())};
	}

	auto index = 0;

	thread_local std::vector<std::pair<Expr*, u32>> named_replace_list;

	named_replace_list.clear();

	for (auto *arg : expr->arguments)
	{
		Type *ctx_type = nullptr;

		if (index < expr->prototype->signature.parameters.size())
		{
			ctx_type = expr->prototype->signature.parameters[index++]->type;
		}

		arg->accept(this, {ctx_type});

		if (auto *named_param = dynamic_cast<NamedExpr*>(arg))
		{
			// this would not scale well but for the average number of function arguments it should still be fairly fast
			for (auto i = 0; auto *param : expr->prototype->signature.parameters)
			{
				if (param->symbol.name == named_param->name)
				{
					named_replace_list.emplace_back(named_param->value, i);
				}

				i++;
			}
		}
	}

	for (auto [named_expr, position] : named_replace_list)
	{
		auto *named = dynamic_cast<NamedExpr*>(expr->arguments[position]);

		if (named == nullptr)
		{
			throw FrontendError{named_expr->token, "named parameter position has already been fulfilled"};
		}

		expr->arguments[position] = named_expr;
	}

	for (auto i = index; i < expr->prototype->signature.parameters.size(); i++)
	{
		auto *param = expr->prototype->signature.parameters[i];

		if (param->initializer != nullptr)
		{
			param->initializer->accept(this, {expr->prototype->signature.parameters[i]->type});
		}
	}

	expr->symbol = expr->prototype->symbol.name;
	expr->type = expr->prototype->signature.return_type;
}

void pars::Analyzer::visit(FnDecl *fn, VisitCtx ctx)
{
	// already been analyzed
	if (fn->signature.return_type != nullptr)
	{
		return;
	}

	m_ctx->scope_table.add_to_scope(fn->symbol, fn, !has_flag(fn->flags, FnFlags::Private));

	auto scope = m_ctx->scope_table.new_scope();

	for (auto *param : fn->signature.parameters)
	{
		param->accept(this, {});
	}

	m_function_stack.emplace_back(fn);

	if (has_flag(fn->flags, FnFlags::ArrowFn))
	{
		auto *expr = dynamic_cast<Expr*>(fn->body->nodes.front());

		// TODO resolve return type if manually typed
		expr->accept(this, {fn->signature.return_type});

		fn->signature.return_type = expr->type;
	}
	else
	{
		fn->signature.return_type = resolve_type(fn->signature.return_type_meta, fn);

		if (fn->body != nullptr)
		{
			for (auto *node : fn->body->nodes)
			{
				node->accept(this, ctx);
			}
		}

		auto flags = m_ctx->scope_table.get_scope_data(m_ctx->scope_table.get_level()).flags;

		if (
			!fn->signature.return_type->is_equal(&VoidType)
			&&
			!has_flag(flags, ScopeFlags::HasReturn)
			&&
			!has_flag(fn->flags, FnFlags::Extern))
		{
			throw FrontendError{fn->token, "Not all paths return a value"};
		}
	}

	m_function_stack.pop_back();
}

void pars::Analyzer::visit(VarDeclStmt *stmt, VisitCtx ctx)
{
	// var x: T
	if (stmt->is_explicitly_typed())
	{
		stmt->type = resolve_type(stmt->type_meta, stmt);
	}

	// var x = E
	if (stmt->initializer != nullptr)
	{
		stmt->initializer->accept(this, {stmt->type});

		// var x = {}
		if (stmt->initializer->type == nullptr)
		{
			throw FrontendError{stmt->token, "Cannot infer type from initializer"};
		}

		if (!stmt->is_explicitly_typed())
		{
			stmt->type = stmt->initializer->type;
		}
	}

	// var x: T = E
	if (stmt->initializer != nullptr && stmt->is_explicitly_typed() && !stmt->initializer->type->is_equal(stmt->type))
	{
		throw FrontendError
		{
			stmt->token,
			fmt::format
			(
				"cannot initialize variable {} of type {} with type {}",
				stmt->symbol.name, stmt->type->get_type_name(), stmt->initializer->type->get_type_name()
			)
		};
	}

	if (dynamic_cast<Pointer*>(stmt->type))
	{
		stmt->type_meta.flags |= TypeFlags::Pointer;
	}

	if (has_keyword_attribute(stmt->symbol, Volatile))
	{
		stmt->flags |= VarFlags::Volatile;
	}

	m_ctx->scope_table.add_to_scope(stmt->symbol, stmt);
}

void pars::Analyzer::visit(ImportStmt *stmt, VisitCtx ctx)
{
	stmt->module = get_module(stmt->path);

	if (stmt->module == nullptr)
	{
		throw FrontendError{stmt->token,
			fmt::format("Could not read module in any include paths '{}'", stmt->path.c_str()), stmt};
	}

	if (!stmt->alias.empty())
	{
		m_ctx->scope_table.add_to_scope(Symbol{stmt->alias}, stmt, PRIVATE_SYMBOL);
	}
	else if (stmt->selective_imports.empty())
	{
		m_ctx->scope_table.add_import(stmt->module->ast.get_file_id());
	}

	for (auto [import_name, symbol_name] : stmt->selective_imports)
	{
		auto *symbol = stmt->module->ast.get_ctx()->scope_table.find_local_symbol(import_name);

		m_ctx->scope_table.add_to_scope(Symbol{symbol_name}, symbol, PRIVATE_SYMBOL);
	}
}

void pars::Analyzer::visit(ReturnStmt *stmt, VisitCtx ctx)
{
	auto *fn = get_current_fn();

	stmt->expr->accept(this, {fn->signature.return_type});

	if (!fn->signature.return_type->is_equal(stmt->expr->type))
	{
		throw FrontendError{stmt->token, fmt::format("Expected {} in return statement but got {}",
			fn->signature.return_type->get_type_name(), stmt->expr->type->get_type_name())};
	}

	m_ctx->scope_table.get_current_flags() |= ScopeFlags::HasReturn;

}

void pars::Analyzer::visit(BlockStmt *stmt, VisitCtx ctx)
{
	auto scope = m_ctx->scope_table.new_scope();

	for (auto *node : stmt->nodes)
	{
		node->accept(this, ctx);
	}

	auto &current_flags = m_ctx->scope_table.get_current_flags();

	if (has_flag(current_flags, ScopeFlags::HasReturn) && m_ctx->scope_table.get_level() > 1)
	{
		auto &data = m_ctx->scope_table.get_scope_data(m_ctx->scope_table.get_level() - 1);
		data.flags|= ScopeFlags::HasReturn;
	}
}

void pars::Analyzer::visit(AssignmentStmt *stmt, VisitCtx ctx)
{
	stmt->lhs->accept(this, {});
	stmt->rhs->accept(this, {stmt->lhs->type});

	// if (!stmt->lhs->type->is_equal(stmt->rhs->type))
	// {
	// 	throw FrontendError{stmt->token,
	// 		fmt::format("assignment to type of {} cannot be done with type of {}",
	// 			stmt->lhs->type->get_type_name(), stmt->rhs->type->get_type_name())};
	// }

	if (auto *symbol = dynamic_cast<SymbolExpr*>(stmt->lhs))
	{
		if (auto *var = dynamic_cast<VarDeclStmt*>(symbol->symbol_node))
		{
			var->flags |= VarFlags::ShouldAlloca;

			if (has_flag(var->type_meta.flags, TypeFlags::Const))
			{
				throw FrontendError(var->token, fmt::format("Cannot mutate const variable '{}'", var->symbol.name));
			}
		}
	}
}

void pars::Analyzer::visit(IfStmt *stmt, VisitCtx ctx)
{
	if (m_ctx->scope_table.get_level() == 0)
	{
		throw FrontendError{stmt->token, "none compile time if statements are not allowed in the global scope"};
	}

	stmt->condition->accept(this, {});

	if (!stmt->condition->type->is_equal(&BoolType))
	{
		throw FrontendError{stmt->condition->token, "if statement condition must be a bool type"};
	}

	stmt->body->accept(this, {});

	// flag will be set by lower scope. disable and only set again if else also has a return
	m_ctx->scope_table.get_current_flags() &= ~ScopeFlags::HasReturn;

	auto sibling_returns = has_flag(m_ctx->scope_table.get_lower_flags(), ScopeFlags::HasReturn);

	if (stmt->else_br != nullptr)
	{
		stmt->else_br->accept(this, {});

		auto else_returns = has_flag(m_ctx->scope_table.get_lower_flags(), ScopeFlags::HasReturn);

		if (sibling_returns && else_returns)
		{
			m_ctx->scope_table.get_scope_data(m_ctx->scope_table.get_level()).flags |= ScopeFlags::HasReturn;
		}
	}
}

void pars::Analyzer::visit(CompIfStmt *stmt, VisitCtx ctx)
{
	stmt->stmt->accept(this, ctx);
}

void pars::Analyzer::visit(WhileStmt *stmt, VisitCtx ctx)
{
	if (m_ctx->scope_table.get_level() == 0)
	{
		throw FrontendError{stmt->token, "none compile time while loops are not allowed in the global scope"};
	}

	stmt->condition->accept(this, {});
	stmt->body->accept(this, {});
}

void pars::Analyzer::visit(ForStmt *stmt, VisitCtx ctx)
{
	stmt->iterable->accept(this, {});

	if (!stmt->iterable->type->is_iterable())
	{
		throw FrontendError{stmt->iterable->token,
			fmt::format("type '{}' is not iterable", stmt->iterable->type->get_type_name())};
	}

	auto max_bindings = stmt->iterable->type->get_iter_bindings().size() + 1;

	if (stmt->bindings.size() > max_bindings)
	{
		throw FrontendError{stmt->iterable->token,
			fmt::format("Too many symbols to bind to, expected at most {} symbols", max_bindings)};
	}

	auto bind = [this](VarDeclStmt *var, Type *type)
	{
		var->type = type;
		var->flags |= VarFlags::ShouldAlloca;

		m_ctx->scope_table.add_to_scope(var->symbol, var, PRIVATE_SYMBOL, m_ctx->scope_table.get_level() + 1);
	};

	for (auto i = 0; auto *type : stmt->iterable->type->get_iter_bindings())
	{
		bind(stmt->bindings[i], type);
		i++;
	}

	if (stmt->has_index())
	{
		auto *last = stmt->bindings.back();

		bind(last, const_cast<Integer*>(&I32Type));
	}

	stmt->body->accept(this, {});
}

void pars::Analyzer::visit(AliasType *alias, VisitCtx ctx)
{
	alias->type = resolve_type(alias->meta, alias);

	// TODO handle private aliasing
	m_ctx->scope_table.add_to_scope(alias->symbol, alias);
}

void pars::Analyzer::visit(SymbolExpr *expr, VisitCtx ctx)
{
	auto *symbol = find_symbol(expr->symbol, expr->token);

	if (auto *var = dynamic_cast<VarDeclStmt*>(symbol))
	{
		expr->type = var->type;
	}
	else if (auto *type = dynamic_cast<Type*>(symbol))
	{
		expr->type = type;
	}

	expr->symbol_node = symbol;
}

void pars::Analyzer::visit(BinaryExpr *expr, VisitCtx ctx)
{
	expr->left->accept(this, ctx);
	expr->right->accept(this, ctx);

	if (!expr->left->type->is_equal(expr->right->type))
	{
		throw FrontendError{expr->token, "binary expression operands types do not match"};
	}

	// TODO ask the type what the result of operators should be

	if (expr->op > _ComparisonStart && expr->op < _ComparisonEnd)
	{
		expr->type = const_cast<Bool*>(&BoolType);
	}
	// range
	else if (expr->op == DotDot || expr->op == DotDotEqual)
	{
		expr->type = const_cast<RangeType*>(&Range);
	}
	else
	{
		expr->type = expr->left->type;
	}
}

void pars::Analyzer::visit(UnaryExpr *expr, VisitCtx ctx)
{
	expr->right->accept(this, ctx);

	if (expr->op == '!')
	{
		expr->type = const_cast<Bool*>(&BoolType);
	}
	else
	{
		expr->type = expr->right->type;
	}
}

void pars::Analyzer::visit(GroupExpr* expr, VisitCtx ctx)
{
	expr->inner->accept(this, ctx);
	expr->type = expr->inner->type;
}

void pars::Analyzer::visit(SizeofExpr* expr, VisitCtx ctx)
{
	expr->expr->accept(this, ctx);
}

void pars::Analyzer::visit(MemberAccessExpr* expr, VisitCtx ctx)
{
	auto *symbol = find_symbol(expr->target_symbol, expr->token);

	if (auto *import = dynamic_cast<ImportStmt*>(symbol))
	{
		auto *old_ctx = m_ctx;

		m_ctx = import->module->ast.get_ctx();

		expr->accessor->accept(this, ctx);

		m_ctx = old_ctx;
	}
	if (auto *type = dynamic_cast<Type*>(symbol))
	{
		auto *prop_expr = new_node<TypePropExpr>();

		auto *prop_symbol = dynamic_cast<SymbolExpr*>(expr->accessor);

		if (prop_symbol == nullptr)
		{
			throw FrontendError{symbol->token, "Expected identifier for type property access"};
		}

		prop_expr->property_name = prop_symbol->symbol;

		prop_expr->type = type;

		expr->accessor = prop_expr;
		prop_expr->token = expr->token;
	}
	else
	{
		expr->accessor->accept(this, ctx);
	}

	expr->type = expr->accessor->type;
}

void pars::Analyzer::visit(CastExpr* expr, VisitCtx ctx)
{
	expr->type_expr->accept(this, ctx);

	expr->type = expr->type_expr->type;

	expr->target->accept(this, {expr->type});
	expr->original_type = expr->target->type;
	expr->target->type = expr->type;
}

void pars::Analyzer::visit(AnonInitExpr *expr, VisitCtx ctx)
{
	expr->type = ctx.type;
}

void pars::Analyzer::visit(NamedExpr *expr, VisitCtx ctx)
{
	expr->value->accept(this, ctx);
	expr->type = expr->value->type;
}

void pars::Analyzer::visit(AbsExpr *expr, VisitCtx ctx)
{
	expr->value->accept(this, ctx);
	expr->type = expr->value->type;
}

void pars::Analyzer::visit(PtrOpExpr *expr, VisitCtx ctx)
{
	expr->symbol->accept(this, ctx);

	if (auto *ptr = dynamic_cast<Pointer*>(expr->symbol->type))
	{
		expr->type = const_cast<Type*>(ptr->inner);
	}
	else
	{
		auto *p = new_node<Pointer>();

		p->inner = expr->symbol->type;

		expr->type = p;
	}
}

void pars::Analyzer::visit(PackedExpr *expr, VisitCtx ctx)
{

}

void pars::Analyzer::analyze(const std::vector<Node *> &nodes)
{
	visit_nodes(nodes);
}

pars::FnDecl * pars::Analyzer::get_current_fn()
{
	if (m_function_stack.empty())
	{
		return nullptr;
	}

	return m_function_stack.back();
}

pars::Type* pars::Analyzer::resolve_type(TypeMeta meta, Node *node)
{
	auto *type = m_ctx->scope_table.find_symbol<Type>(meta.name);

	if (type == nullptr)
	{
		throw FrontendError{node->token, fmt::format("unknown type name '{}'", meta.name), nullptr};
	}

	if (has_flag(meta.flags, TypeFlags::Pointer))
	{
		auto *ptr = new_node<Pointer>();

		ptr->inner = type;

		type = ptr;
	}

	return type;
}

void pars::Analyzer::add_symbol_task(Type *type, std::string_view symbol, SymbolTask &&task)
{
	if (type != nullptr)
	{
		task.fn(task.node);
	}
	else
	{
		m_symbol_tasks[symbol] = std::move(task);
	}
}

pars::Node* pars::Analyzer::find_symbol(std::string_view name, Token &error_token)
{
	auto *symbol = m_ctx->scope_table.find_symbol(name);

	if (symbol == nullptr)
	{
		throw FrontendError{error_token, fmt::format("unknown symbol '{}'", name)};
	}

	return symbol;
}

pars::Analyzer::Analyzer(ParseCtx *parse_ctx)
{
	m_ctx = parse_ctx;
}
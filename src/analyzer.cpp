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

	if (expr->arguments.size() < expr->prototype->signature.callable_arity || expr->arguments.size() > expr->prototype->signature.parameters.size())
	{
		throw FrontendError{expr->token, fmt::format("expected {} argument but got {}",
			expr->prototype->signature.callable_arity, expr->arguments.size())};
	}

	auto index = 0;

	thread_local std::vector<std::pair<Expr*, u32>> named_replace_list;

	named_replace_list.clear();

	for (auto *arg : expr->arguments)
	{
		arg->accept(this, {expr->prototype->signature.parameters[index++]->type});

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
		fn->signature.return_type = resolve_type(fn->signature.return_type_name, fn);

		if (fn->body != nullptr)
		{
			for (auto *node : fn->body->nodes)
			{
				node->accept(this, ctx);
			}
		}
	}

	m_function_stack.pop_back();
}

void pars::Analyzer::visit(VarDeclStmt *stmt, VisitCtx ctx)
{
	// var x: T
	if (!stmt->type_name.empty())
	{
		stmt->type = resolve_type(stmt->type_name, stmt);
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

		if (stmt->type_name.empty())
		{
			stmt->type = stmt->initializer->type;
		}
	}

	// var x: T = E
	if (stmt->initializer != nullptr && !stmt->type_name.empty() && !stmt->initializer->type->is_equal(stmt->type))
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
}

void pars::Analyzer::visit(BlockStmt *stmt, VisitCtx ctx)
{
	auto scope = m_ctx->scope_table.new_scope();

	for (auto *node : stmt->nodes)
	{
		node->accept(this, ctx);
	}
}

void pars::Analyzer::visit(AssignmentStmt *stmt, VisitCtx ctx)
{
	stmt->lhs = find_symbol<VarDeclStmt>(stmt->symbol, stmt->token);

	if (stmt->lhs == nullptr)
	{
		throw FrontendError{stmt->token, fmt::format("'{}' is not a valid target for assignment", stmt->symbol)};
	}

	stmt->rhs->accept(this, {stmt->lhs->type});

	if (!stmt->lhs->type->is_equal(stmt->rhs->type))
	{
		throw FrontendError{stmt->token,
			fmt::format("assignment to type of {} cannot be done with type of {}",
				stmt->lhs->type->get_type_name(), stmt->rhs->type->get_type_name())};
	}

	stmt->lhs->flags |= VarFlags::ShouldAlloca;

	if (has_keyword_attribute(stmt->lhs->symbol, Volatile))
	{
		stmt->lhs->flags |= VarFlags::Volatile;
	}
}

void pars::Analyzer::visit(IfStmt *stmt, VisitCtx ctx)
{
	stmt->condition->accept(this, {});

	if (!stmt->condition->type->is_equal(&BoolType))
	{
		throw FrontendError{stmt->condition->token, "if statement condition must be a bool type"};
	}

	stmt->body->accept(this, {});

	if (stmt->else_br != nullptr)
	{
		stmt->else_br->accept(this, {});
	}
}

void pars::Analyzer::visit(WhileStmt *stmt, VisitCtx ctx)
{
	stmt->condition->accept(this, {});
	stmt->body->accept(this, {});
}

void pars::Analyzer::visit(AliasType *alias, VisitCtx ctx)
{
	alias->type = resolve_type(dynamic_cast<PendingType*>(alias->type)->symbol, alias);

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

	if (expr->op > _ComparisonStart && expr->op < _ComparisonEnd)
	{
		expr->type = const_cast<Bool*>(&BoolType);
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

pars::Type* pars::Analyzer::resolve_type(std::string_view name, Node *node)
{
	auto *type = m_ctx->scope_table.find_symbol<Type>(name);

	if (type == nullptr)
	{
		throw FrontendError{node->token, fmt::format("unknown type name '{}'", name), nullptr};
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

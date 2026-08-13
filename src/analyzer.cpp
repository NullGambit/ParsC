#include "analyzer.hpp"

#include <ranges>

#include "parse_ctx.hpp"
#include "ast.hpp"
#include "frontend_error.hpp"
#include "module.hpp"
#include "module_manager.hpp"
#include "util/fmt.hpp"

using enum pars::TokenType;

void pars::Analyzer::visit(CallExpr *expr, VisitCtx ctx)
{
	ScopeTable *table_override {};

	if (ctx.parse_ctx_override != nullptr)
	{
		table_override = &ctx.parse_ctx_override->scope_table;
	}

	expr->prototype = find_symbol<FnDecl>(expr->symbol, expr->token, table_override);

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
	expr->const_set = expr->prototype->signature.return_type_meta.const_set;
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
				if (auto *expr = dynamic_cast<Expr*>(node))
				{
					visit_expr(nullptr, expr, ctx);
				}
				else
				{
					node->accept(this, ctx);
				}
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

void pars::Analyzer::visit(Struct *stmt, VisitCtx ctx)
{
	// already been resolved
	if (!stmt->fields.empty() && stmt->fields.front().type != nullptr)
	{
		return;
	}

	for (auto &field : stmt->fields)
	{
		field.type = resolve_type(field.type_meta, stmt);
	}
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

		if (stmt->type_meta.type == nullptr)
		{
			stmt->type_meta.const_set = stmt->initializer->const_set;
		}
	}

	if (auto *array = dynamic_cast<Array*>(stmt->type); array && array->size == UNSIZED_ARRAY && stmt->initializer == nullptr)
	{
		throw FrontendError{stmt->token, "Cannot infer size of array"};
	}

	// var x: T = E
	if (stmt->initializer != nullptr && stmt->is_explicitly_typed() && !is_assignable_from(stmt->initializer->type, stmt->type))
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

	// if (stmt->initializer != nullptr)
	// {
	// 	stmt->type = stmt->initializer->type;
	// }

	if (has_keyword_attribute(stmt->symbol, Volatile))
	{
		stmt->flags |= VarFlags::Volatile;
	}

	if (m_ctx->scope_table.get_level() == 0)
	{
		stmt->flags |= VarFlags::Global;
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
	if (stmt->expr== nullptr)
	{
		return;
	}

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
		if (auto *expr = dynamic_cast<Expr*>(node))
		{
			visit_expr(nullptr, expr, ctx);
		}
		else
		{
			node->accept(this, ctx);
		}
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
	visit_expr(nullptr, stmt->lhs, {});
	visit_expr(nullptr, stmt->rhs, {stmt->lhs->type});

	if (!stmt->lhs->type->is_equal(stmt->rhs->type) && !stmt->lhs->type->can_coerce_into(stmt->rhs->type))
	{
		throw FrontendError{stmt->token,
			fmt::format("assignment to type of {} cannot be done with type of {}",
				stmt->lhs->type->get_type_name(), stmt->rhs->type->get_type_name())};
	}
}

void pars::Analyzer::visit(IfStmt *stmt, VisitCtx ctx)
{
	if (m_ctx->scope_table.get_level() == 0)
	{
		throw FrontendError{stmt->token, "none compile time if statements are not allowed in the global scope"};
	}

	visit_expr(nullptr, stmt->condition, {});

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

	visit_expr(nullptr, stmt->condition, {});
	stmt->body->accept(this, {});
}

void pars::Analyzer::visit(ForStmt *stmt, VisitCtx ctx)
{
	visit_expr(nullptr, stmt->iterable, {});

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

	m_ctx->scope_table.add_to_scope(alias->symbol, alias, false);
}

void pars::Analyzer::visit(SymbolExpr *expr, VisitCtx ctx)
{
	if (ctx.member)
	{
		auto maybe_member = expr->type->get_member(expr->symbol);

		if (maybe_member.has_value())
		{
			auto member = maybe_member.value();

			expr->type = member.type;
		}
	}
	else
	{
		auto *symbol = find_symbol(expr->symbol, expr->token);

		if (auto *var = dynamic_cast<VarDeclStmt*>(symbol))
		{
			expr->const_set = var->type_meta.const_set;
			expr->type = var->type;
		}
		else if (auto *type = dynamic_cast<Type*>(symbol))
		{
			expr->type = type;
		}

		expr->symbol_node = symbol;
	}

}

void pars::Analyzer::visit(BinaryExpr *expr, VisitCtx ctx)
{
	visit_expr(expr, expr->left, ctx);
	visit_expr(expr, expr->right, ctx);

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
		// TODO ask the type what the result of operators should be
		if (!expr->left->type->is_equal(expr->right->type))
		{
			throw FrontendError{expr->token, "binary expression operands types do not match"};
		}

		expr->type = expr->left->type;
	}
}

void pars::Analyzer::visit(UnaryExpr *expr, VisitCtx ctx)
{
	visit_expr(expr, expr->right, ctx);

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
	visit_expr(expr, expr->inner, ctx);
	expr->type = expr->inner->type;
}

void pars::Analyzer::visit(SizeofExpr* expr, VisitCtx ctx)
{
	visit_expr(expr, expr->expr, ctx);
}

void pars::Analyzer::visit(MemberAccessExpr* expr, VisitCtx ctx)
{
	// my_import.func()
	// i32.max
	// my_struct.field
	// (my_struct).field
	// array.length
	// func().field

	auto symbol = expr->target->get_symbol();

	auto *symbol_node = ctx.member ? nullptr : find_symbol(symbol, expr->token);

	if (auto *import = dynamic_cast<ImportStmt*>(symbol_node))
	{
		ctx.parse_ctx_override = import->module->ast.get_ctx();

		visit_expr(expr, expr->accessor, ctx);
	}
	else if (auto *type = dynamic_cast<Type*>(symbol_node))
	{
		auto *prop_expr = new_node<TypePropExpr>();

		auto *prop_symbol = dynamic_cast<SymbolExpr*>(expr->accessor);

		if (prop_symbol == nullptr)
		{
			throw FrontendError{symbol_node->token, "Expected identifier for type property access"};
		}

		prop_expr->property_name = prop_symbol->symbol;

		prop_expr->type = type;

		expr->accessor = prop_expr;
		prop_expr->token = expr->token;
	}
	else
	{
		if (expr->type == nullptr)
		{
			visit_expr(expr, expr->target, ctx);
		}
		else
		{
			expr->target->type = expr->type;
		}

		auto subsymbol = expr->accessor->get_symbol();

		auto maybe_member = expr->target->type->get_member(subsymbol);

		// a.b.c
		if (!maybe_member.has_value())
		{
			throw FrontendError{expr->token, fmt::format("member '{}' does not exist on '{}' object",
				subsymbol, expr->target->get_symbol())};
		}

		expr->accessor->type = maybe_member.value().type;
		ctx.member = true;

		visit_expr(expr, expr->accessor, ctx);
	}

	expr->type = expr->accessor->type;
}

void pars::Analyzer::visit(CastExpr* expr, VisitCtx ctx)
{
	visit_expr(expr, expr->type_expr, ctx);

	expr->type = expr->type_expr->type;

	visit_expr(expr, expr->target, {expr->type});

	expr->original_type = expr->target->type;
	expr->target->type = expr->type;
}

void pars::Analyzer::visit(AnonInitExpr *expr, VisitCtx ctx)
{
	expr->type = ctx.type;

	if (auto *struct_type = dynamic_cast<Struct*>(expr->type))
	{
		assign_struct_indices(struct_type, expr->values);
	}
}

void pars::Analyzer::visit(NamedExpr *expr, VisitCtx ctx)
{
	visit_expr(expr, expr->value, ctx);
	expr->type = expr->value->type;
}

void pars::Analyzer::visit(AbsExpr *expr, VisitCtx ctx)
{
	visit_expr(expr, expr->value, ctx);
	expr->type = expr->value->type;
}

void pars::Analyzer::visit(PtrOpExpr *expr, VisitCtx ctx)
{
	visit_expr(expr, expr->target, ctx);

	expr->const_set = expr->target->const_set;

	switch (expr->op)
	{
		case Caret:
		{
			if (!expr->target->type->is_ptr())
			{
				throw FrontendError{expr->token, "Dereference target is not a pointer"};
			}

			expr->type = expr->target->type->get_inner();

			break;
		}
		case Ampersand:
		{
			auto *p = new_node<Pointer>();

			p->inner = expr->target->type;

			expr->type = p;

			break;
		}
	}
}

void pars::Analyzer::visit(PackedExpr *expr, VisitCtx ctx)
{

}

void pars::Analyzer::visit(ArrayLiteralExpr *expr, VisitCtx ctx)
{
	if (expr->elements.empty())
	{
		return;
	}

	auto *type = ctx.type;

	if (type != nullptr)
	{
		type = type->get_inner();
		ctx.type = type;
	}

	for (auto *element : expr->elements)
	{
		visit_expr(expr, element, ctx);

		if (type == nullptr)
		{
			type = element->type;
			ctx.type = type;
		}

		if (!type->is_equal(element->type))
		{
			throw FrontendError{element->token, "array literal element types dont all match"};
		}
	}

	auto *array_type = new_node<Array>();

	array_type->size = expr->elements.size();
	array_type->element_type = type;

	expr->type = array_type;

	if (ctx.type != nullptr && !type->is_equal(ctx.type))
	{
		throw FrontendError{expr->token, "got wrong array type"};
	}
}

void pars::Analyzer::visit(IndexOpExpr *expr, VisitCtx ctx)
{
	visit_expr(expr, expr->lhs, ctx);
	visit_expr(expr, expr->index, ctx);

	if (dynamic_cast<Integer*>(expr->index->type) == nullptr)
	{
		throw FrontendError{expr->index->token, "Array index must be an integer"};
	}

	if (!expr->lhs->type->is_array())
	{
		throw FrontendError{expr->lhs->token, "left hand side is not an array"};
	}

	expr->type = expr->lhs->type->get_inner();
}

void pars::Analyzer::visit(StructLiteral *expr, VisitCtx ctx)
{
	expr->type = get_type(expr->name, expr->token);

	auto *struct_type = dynamic_cast<Struct*>(expr->type);

	assign_struct_indices(struct_type, expr->initializers);
}

void pars::Analyzer::visit(SliceExpr *expr, VisitCtx ctx)
{
	visit_expr(expr, expr->lhs, ctx);

	if (expr->start != nullptr)
	{
		visit_expr(expr, expr->start, ctx);
	}

	if (expr->end != nullptr)
	{
		visit_expr(expr, expr->end, ctx);
	}

	if (!expr->lhs->type->is_array())
	{
		throw FrontendError{expr->token, fmt::format("type of {} cannot be sliced", expr->type->get_type_name())};
	}

	auto *slice_type = new_node<Slice>();

	slice_type->element_type = expr->lhs->type->get_inner();

	expr->type = slice_type;
}

void pars::Analyzer::visit(UnresolvedSymbol *type, VisitCtx ctx)
{
	*ctx.result = get_type(type->symbol, type->token);
}

void pars::Analyzer::visit(Pointer *type, VisitCtx ctx)
{
	type->inner->accept(this, {.result = (Node**)&type->inner});
}

void pars::Analyzer::visit(BaseArray *type, VisitCtx ctx)
{
	type->element_type->accept(this, {.result = (Node**)&type->element_type});
}

void pars::Analyzer::visit(Array *type, VisitCtx ctx)
{
	type->element_type->accept(this, {.result = (Node**)&type->element_type});

	visit_expr(nullptr, type->size_expr, ctx);
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

pars::Type* pars::Analyzer::resolve_type(TypeMeta &meta, Node *node)
{
	if (meta.type != nullptr)
	{
		meta.type->accept(this, {.result = (Node**)&meta.type});
	}

	if (auto *alias = dynamic_cast<AliasType*>(meta.type))
	{
		meta.const_set = alias->meta.const_set;
	}

	return meta.type;
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

pars::Node* pars::Analyzer::find_symbol(std::string_view name, Token &error_token, ScopeTable *table_override)
{
	auto *scope_table = table_override != nullptr ? table_override : &m_ctx->scope_table;
	auto *symbol = scope_table->find_symbol(name);

	if (symbol == nullptr)
	{
		throw FrontendError{error_token, fmt::format("unknown symbol '{}'", name)};
	}

	return symbol;
}

void pars::Analyzer::assign_struct_indices(const Struct *struct_type, std::vector<std::pair<Expr *, u32>> &initializers)
{
	auto cursor = 0;

	for (auto i = 0; i < struct_type->fields.size(); i++)
	{
		if (i >= initializers.size())
		{
			break;
		}

		auto &[initializer, pos] = initializers[i];

		if (auto *named_expr = dynamic_cast<NamedExpr*>(initializer))
		{
			auto it = std::ranges::find_if(struct_type->fields, [&](const StructField &field)
			{
				// TODO perhaps implementing a string interning system within the compiler to speed up this comparison
				// or see if llvm has one i can already use
				// or use hashes
				return named_expr->name == field.symbol.name;
			});

			pos = std::distance(struct_type->fields.begin(), it);
			cursor = pos;
		}
		else
		{
			pos = i;
		}

		auto &field = struct_type->fields[cursor];
		auto *expected_type = field.type;

		initializer->accept(this, {.type = expected_type});

		if (!expected_type->is_equal(initializer->type))
		{
			throw FrontendError{initializer->token,
				fmt::format("cannot initialize struct field {} of type {} with type {}",
					field.symbol.name,
					expected_type->get_type_name(),
					initializer->type->get_type_name())};
		}

		pos = cursor;
		cursor++;
	}
}

pars::Type * pars::Analyzer::get_type(std::string_view name, Token &error_token)
{
	auto *type = m_ctx->scope_table.find_symbol<Type>(name);

	if (type == nullptr)
	{
		throw FrontendError{error_token, fmt::format("unknown type name '{}'", name), nullptr};
	}

	return type;
}

void pars::Analyzer::visit_expr(Expr *parent, Expr *expr, VisitCtx ctx)
{
	ctx.depth = m_expr_depth++;

	expr->accept(this, ctx);

	if (expr->const_set.test(ctx.depth))
	{
		expr->flags |= ExprFlags::Immutable;
	}

	if (parent != nullptr)
	{
		parent->const_set = expr->const_set;
	}

	m_expr_depth--;
}

pars::Analyzer::Analyzer(ParseCtx *parse_ctx)
{
	m_ctx = parse_ctx;
}

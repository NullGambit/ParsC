#include "analyzer.hpp"

#include <ranges>

#include "parse_ctx.hpp"
#include "ast.hpp"
#include "frontend_error.hpp"
#include "module.hpp"
#include "module_manager.hpp"
#include "util/fmt.hpp"

using enum pars::TokenType;

pars::Node* pars::Analyzer::visit(CallExpr *expr, VisitCtx ctx)
{
	ScopeTable *table_override {};

	if (ctx.parse_ctx_override != nullptr)
	{
		table_override = &ctx.parse_ctx_override->scope_table;
	}

	// set type from the expr in case of being called from a member access
	expr->callable->type = expr->type;

	expr->callable->accept(this, ctx);

	auto maybe_call_info = expr->callable->type->get_call_info();

	if (!maybe_call_info.has_value())
	{
		throw FrontendError{expr->token, fmt::format("{} is not callable", expr->callable->get_symbol())};
	}

	auto call_info = maybe_call_info.value();

	if (!call_info.is_variadic &&
		(expr->arguments.size() < call_info.callable_arity ||
		expr->arguments.size() > call_info.parameters.size()))
	{
		throw FrontendError{expr->token, fmt::format("expected {} argument but got {}",
			call_info.callable_arity, expr->arguments.size())};
	}

	auto index = 0;

	thread_local std::vector<std::pair<Expr*, u32>> named_replace_list;

	named_replace_list.clear();

	for (auto *arg : expr->arguments)
	{
		Type *ctx_type = nullptr;

		if (index < call_info.parameters.size())
		{
			ctx_type = call_info.parameters[index++]->type;
		}

		arg = visit_expr(expr, arg, {ctx_type});

		if (auto *named_param = dynamic_cast<NamedExpr*>(arg))
		{
			// this would not scale well but for the average number of function arguments it should still be fairly fast
			for (auto i = 0; auto *param : call_info.parameters)
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

	for (auto i = index; i < call_info.parameters.size(); i++)
	{
		auto *param = call_info.parameters[i];

		if (param->initializer != nullptr)
		{
			param->initializer = visit_expr(expr, param->initializer, {call_info.parameters[i]->type});
		}
	}

	expr->type = call_info.return_meta.type;
	expr->mut_set = call_info.return_meta.mut_set;

	return expr;
}

pars::Node* pars::Analyzer::visit(FnType *fn, VisitCtx ctx)
{
	// already been analyzed
	if (fn->signature.return_type != nullptr)
	{
		return fn;
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
		fn->signature.return_type_meta.type = expr->type;
	}
	else
	{
		fn->signature.return_type = resolve_type(fn->signature.return_type_meta, fn);
		fn->signature.return_type_meta.type = fn->signature.return_type;

		if (fn->body != nullptr)
		{
			for (auto i = 0; auto *node : fn->body->nodes)
			{
				if (auto *expr = dynamic_cast<Expr*>(node))
				{
					fn->body->nodes[i] = visit_expr(nullptr, expr, ctx);
				}
				else
				{
					node->accept(this, ctx);
				}

				i += 1;
			}
		}

		auto flags = m_ctx->scope_table.get_scope_data(m_ctx->scope_table.get_level()).flags;

		if (
			!fn->signature.return_type->is_equal(&VoidType)
			&&
			!has_flag(flags, ScopeFlags::HasReturn)
			&&
			fn->body != nullptr)
		{
			throw FrontendError{fn->token, "Not all paths return a value"};
		}
	}

	m_function_stack.pop_back();

	return fn;
}

pars::Node* pars::Analyzer::visit(Struct *stmt, VisitCtx ctx)
{
	// already been resolved
	if (!stmt->fields.empty() && stmt->fields.front().type != nullptr)
	{
		return stmt;
	}

	for (auto &field : stmt->fields)
	{
		field.type = resolve_type(field.type_meta, stmt);
	}

	return stmt;
}

pars::Node* pars::Analyzer::visit(VarDeclStmt *stmt, VisitCtx ctx)
{
	// var x: T
	if (stmt->is_explicitly_typed())
	{
		stmt->type = resolve_type(stmt->type_meta, stmt);
	}

	// var x = E
	if (stmt->initializer != nullptr)
	{
		stmt->initializer = visit_expr(nullptr, stmt->initializer, {.type = stmt->type});

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
			auto top_level_mut = stmt->type_meta.mut_set.test(0);

			stmt->type_meta.mut_set = stmt->initializer->mut_set;

			if (top_level_mut)
			{
				stmt->type_meta.mut_set.set(0);
			}
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

	if (has_flag(stmt->flags, VarFlags::Const))
	{
		if (stmt->initializer == nullptr)
		{
			throw FrontendError{stmt->token, "const must have an initializer"};
		}

		stmt->initializer = dynamic_cast<Expr*>(stmt->initializer->accept(&m_comp_eval, ctx));

		if (stmt->initializer == nullptr)
		{
			throw FrontendError{stmt->token, "const must have an initializer known at compile time"};
		}
	}

	m_ctx->scope_table.add_to_scope(stmt->symbol, stmt);

	return stmt;
}

pars::Node* pars::Analyzer::visit(ImportStmt *stmt, VisitCtx ctx)
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

	return stmt;
}

pars::Node* pars::Analyzer::visit(ReturnStmt *stmt, VisitCtx ctx)
{
	if (stmt->expr== nullptr)
	{
		return stmt;
	}

	auto *fn = get_current_fn();

	stmt->expr->accept(this, {fn->signature.return_type});

	if (!fn->signature.return_type->is_equal(stmt->expr->type))
	{
		throw FrontendError{stmt->token, fmt::format("Expected {} in return statement but got {}",
			fn->signature.return_type->get_type_name(), stmt->expr->type->get_type_name())};
	}

	m_ctx->scope_table.get_current_flags() |= ScopeFlags::HasReturn;

	return stmt;
}

pars::Node* pars::Analyzer::visit(BlockStmt *stmt, VisitCtx ctx)
{
	auto scope = m_ctx->scope_table.new_scope();

	for (auto i = 0; auto *node : stmt->nodes)
	{
		if (auto *expr = dynamic_cast<Expr*>(node))
		{
			stmt->nodes[i] = visit_expr(nullptr, expr, ctx);
		}
		else
		{
			node->accept(this, ctx);
		}

		i += 1;
	}

	auto &current_flags = m_ctx->scope_table.get_current_flags();

	if (has_flag(current_flags, ScopeFlags::HasReturn) && m_ctx->scope_table.get_level() > 1)
	{
		auto &data = m_ctx->scope_table.get_scope_data(m_ctx->scope_table.get_level() - 1);
		data.flags|= ScopeFlags::HasReturn;
	}

	return stmt;
}

pars::Node* pars::Analyzer::visit(AssignmentStmt *stmt, VisitCtx ctx)
{
	stmt->lhs = visit_expr(nullptr, stmt->lhs, {});
	stmt->rhs = visit_expr(nullptr, stmt->rhs, {stmt->lhs->type});

	if (!stmt->lhs->type->is_equal(stmt->rhs->type) && !stmt->lhs->type->can_coerce_into(stmt->rhs->type))
	{
		throw FrontendError{stmt->token,
			fmt::format("assignment to type of {} cannot be done with type of {}",
				stmt->lhs->type->get_type_name(), stmt->rhs->type->get_type_name())};
	}

	return stmt;
}

pars::Node* pars::Analyzer::visit(IfStmt *stmt, VisitCtx ctx)
{
	if (m_ctx->scope_table.get_level() == 0)
	{
		throw FrontendError{stmt->token, "none compile time if statements are not allowed in the global scope"};
	}

	stmt->condition = visit_expr(nullptr, stmt->condition, {});

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

	return stmt;
}

pars::Node* pars::Analyzer::visit(CompIfStmt *stmt, VisitCtx ctx)
{
	stmt->stmt->accept(this, ctx);

	return stmt;
}

pars::Node* pars::Analyzer::visit(WhileStmt *stmt, VisitCtx ctx)
{
	if (m_ctx->scope_table.get_level() == 0)
	{
		throw FrontendError{stmt->token, "none compile time while loops are not allowed in the global scope"};
	}

	stmt->condition = visit_expr(nullptr, stmt->condition, {});
	stmt->body->accept(this, {});

	return stmt;
}

pars::Node* pars::Analyzer::visit(ForStmt *stmt, VisitCtx ctx)
{
	stmt->iterable = visit_expr(nullptr, stmt->iterable, {});

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

	return stmt;
}

pars::Node* pars::Analyzer::visit(AliasType *alias, VisitCtx ctx)
{
	alias->type = resolve_type(alias->meta, alias);

	m_ctx->scope_table.add_to_scope(alias->symbol, alias, false);

	return alias;
}

pars::Node* pars::Analyzer::visit(SymbolExpr *expr, VisitCtx ctx)
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
			expr->mut_set = var->type_meta.mut_set;
			expr->type = var->type;
		}
		else if (auto *fn = dynamic_cast<FnType*>(symbol))
		{
			expr->type = fn;
		}
		else if (auto *type = dynamic_cast<Type*>(symbol))
		{
			expr->type = type;
		}

		expr->symbol_node = symbol;
	}

	return expr;
}

pars::Node* pars::Analyzer::visit(BinaryExpr *expr, VisitCtx ctx)
{
	expr->left = visit_expr(expr, expr->left, ctx);
	expr->right = visit_expr(expr, expr->right, ctx);

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

	return expr;
}

pars::Node* pars::Analyzer::visit(UnaryExpr *expr, VisitCtx ctx)
{
	expr->right = visit_expr(expr, expr->right, ctx);

	if (expr->op == '!')
	{
		expr->type = const_cast<Bool*>(&BoolType);
	}
	else
	{
		expr->type = expr->right->type;
	}

	return expr;
}

pars::Node* pars::Analyzer::visit(GroupExpr* expr, VisitCtx ctx)
{
	expr->inner = visit_expr(expr, expr->inner, ctx);
	expr->type = expr->inner->type;

	return expr;
}

pars::Node* pars::Analyzer::visit(SizeofExpr* expr, VisitCtx ctx)
{
	expr->expr = visit_expr(expr, expr->expr, ctx);

	return expr;
}

pars::Node* pars::Analyzer::visit(MemberAccessExpr* expr, VisitCtx ctx)
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

		expr->accessor = visit_expr(expr, expr->accessor, ctx);
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
			expr->target = visit_expr(expr, expr->target, ctx);
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

		expr->accessor = visit_expr(expr, expr->accessor, ctx);
	}

	expr->type = expr->accessor->type;
	expr->mut_set = expr->target->mut_set;

	return expr;
}

pars::Node* pars::Analyzer::visit(CastExpr* expr, VisitCtx ctx)
{
	expr->type_expr = visit_expr(expr, expr->type_expr, ctx);

	expr->type = expr->type_expr->type;

	expr->target = visit_expr(expr, expr->target, {expr->type});

	expr->original_type = expr->target->type;
	expr->target->type = expr->type;

	return expr;
}

pars::Node* pars::Analyzer::visit(NamedExpr *expr, VisitCtx ctx)
{
	expr->value = visit_expr(expr, expr->value, ctx);
	expr->type = expr->value->type;

	return expr;
}

pars::Node* pars::Analyzer::visit(AbsExpr *expr, VisitCtx ctx)
{
	expr->value = visit_expr(expr, expr->value, ctx);
	expr->type = expr->value->type;

	return expr;
}

pars::Node* pars::Analyzer::visit(PtrOpExpr *expr, VisitCtx ctx)
{
	expr->target = visit_expr(expr, expr->target, ctx);

	expr->mut_set = expr->target->mut_set;

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

	return expr;
}

pars::Node* pars::Analyzer::visit(PackedExpr *expr, VisitCtx ctx)
{
	return expr;
}

namespace pars
{
	template<class Fields, class FindFn, class GetTypeFN>
	void assign_named_indices
	(
		const Fields &fields,
		FindFn find_fn, GetTypeFN get_type_fn,
		Analyzer *analyzer,
		InitializerList &initializers
	)
	{
		u32 cursor = 0;

		for (u32 i = 0; i < fields.size(); i++)
		{
			if (i >= initializers.size())
			{
				break;
			}

			auto &[initializer, pos] = initializers[i];

			if (auto *named_expr = dynamic_cast<NamedExpr*>(initializer))
			{
				auto it = std::ranges::find_if(fields, [&](const auto &element) { return find_fn(element, named_expr); });

				pos = std::distance(fields.begin(), it);
				cursor = pos;
			}
			else
			{
				pos = i;
			}

			auto &field = fields[cursor];
			auto *expected_type = get_type_fn(field);

			initializer->accept(analyzer, {.type = expected_type});

			if (!expected_type->is_equal(initializer->type))
			{
				throw FrontendError{initializer->token,
					fmt::format("cannot initialize object at position {} of type {} with type {}",
						i,
						expected_type->get_type_name(),
						initializer->type->get_type_name())};
			}

			pos = cursor;
			cursor++;
		}
	}

	void assign_struct_indices(Struct *type, Analyzer *analyzer, InitializerList &initializers)
	{
		auto find_fn = [](const StructField &field, NamedExpr *named_expr)
		{
			// TODO perhaps implementing a string interning system within the compiler to speed up this comparison
			// or see if llvm has one i can already use
			// or use hashes
			return named_expr->name == field.symbol.name;
		};

		auto get_type_fn = [](const StructField &field) { return field.type; };

		assign_named_indices(type->fields, find_fn, get_type_fn, analyzer, initializers);
	}
}

pars::Node* pars::Analyzer::visit(ArrayLiteralExpr *expr, VisitCtx ctx)
{
	if (expr->initializers.empty())
	{
		return expr;
	}

	auto *array_type = new_node<Array>();

	if (expr->type_specifier != nullptr)
	{
		array_type->element_type = get_type(expr->type_specifier->get_symbol(), expr->type_specifier->token);

		if (auto *alias = dynamic_cast<AliasType*>(array_type->element_type))
		{
			array_type = dynamic_cast<Array*>(alias->type);
		}
	}

	expr->type = array_type;

	if (!array_type->members.empty())
	{
		auto find_fn = [](const std::string_view &name, NamedExpr *named_expr)
		{
			return named_expr->name == name;
		};

		auto get_type_fn = [array_type](const std::string_view &name) { return array_type->element_type; };

		assign_named_indices(array_type->members, find_fn, get_type_fn, this, expr->initializers);

		return expr;
	}

	array_type->size = expr->initializers.size();

	if (expr->type_specifier != nullptr)
	{
		array_type->element_type = get_type(expr->type_specifier->get_symbol(), expr->type_specifier->token);
	}

	ctx.type = array_type->element_type;

	for (auto i = 0; auto &[element, pos] : expr->initializers)
	{
		element = visit_expr(expr, element, ctx);

		if (array_type->element_type == nullptr)
		{
			array_type->element_type = element->type;
		}

		if (!array_type->element_type->is_equal(element->type))
		{
			throw FrontendError{element->token, "array literal element types dont all match"};
		}

		pos = i;
		i += 1;
	}


	if (auto *ctx_array = dynamic_cast<Array*>(ctx.type); ctx_array && ctx_array->size != UNSIZED_ARRAY)
	{
		if (ctx_array->size < array_type->size)
		{
			throw FrontendError{expr->token, "Array has too many members"};
		}

		array_type->size = ctx_array->size;
	}

	return expr;
}

pars::Node* pars::Analyzer::visit(IndexOpExpr *expr, VisitCtx ctx)
{
	auto *left_symbol = find_symbol(expr->lhs->get_symbol(), expr->lhs->token);

	if (dynamic_cast<Type*>(left_symbol))
	{
		auto *literal = new_node<ArrayLiteralExpr>();

		literal->type_specifier = expr->lhs;
		literal->initializers = InitializerList{InitializerElement{expr->index}};

		return visit_expr(expr, literal, ctx);
	}

	expr->lhs = visit_expr(expr, expr->lhs, ctx);
	expr->index = visit_expr(expr, expr->index, ctx);

	if (dynamic_cast<Integer*>(expr->index->type) == nullptr)
	{
		throw FrontendError{expr->index->token, "Array index must be an integer"};
	}

	if (!expr->lhs->type->is_array())
	{
		throw FrontendError{expr->lhs->token, "left hand side is not an array"};
	}

	expr->type = expr->lhs->type->get_inner();

	return expr;
}



pars::Node* pars::Analyzer::visit(StructLiteral *expr, VisitCtx ctx)
{
	expr->type = get_type(expr->name, expr->token);

	auto *struct_type = dynamic_cast<Struct*>(expr->type);

	assign_struct_indices(struct_type, this, expr->initializers);

	return expr;
}

pars::Node* pars::Analyzer::visit(AnonInitExpr *expr, VisitCtx ctx)
{
	expr->type = ctx.type;

	if (auto *struct_type = dynamic_cast<Struct*>(expr->type))
	{
		assign_struct_indices(struct_type, this, expr->values);
	}

	return expr;
}

pars::Node* pars::Analyzer::visit(SliceExpr *expr, VisitCtx ctx)
{
	expr->lhs = visit_expr(expr, expr->lhs, ctx);

	if (expr->start != nullptr)
	{
		expr->start = visit_expr(expr, expr->start, ctx);
	}

	if (expr->end != nullptr)
	{
		expr->end = visit_expr(expr, expr->end, ctx);
	}

	if (!expr->lhs->type->is_array())
	{
		throw FrontendError{expr->token, fmt::format("type of {} cannot be sliced", expr->type->get_type_name())};
	}

	auto *slice_type = new_node<Slice>();

	slice_type->element_type = expr->lhs->type->get_inner();

	expr->type = slice_type;

	return expr;
}

pars::Node* pars::Analyzer::visit(UnresolvedSymbol *type, VisitCtx ctx)
{
	*ctx.result = get_type(type->symbol, type->token);

	return type;
}

pars::Node* pars::Analyzer::visit(Pointer *type, VisitCtx ctx)
{
	type->inner->accept(this, {.result = (Node**)&type->inner});

	return type;
}

pars::Node* pars::Analyzer::visit(BaseArray *type, VisitCtx ctx)
{
	type->element_type->accept(this, {.result = (Node**)&type->element_type});

	return type;
}

pars::Node* pars::Analyzer::visit(Array *type, VisitCtx ctx)
{
	type->element_type->accept(this, {.result = (Node**)&type->element_type});

	type->size_expr = visit_expr(nullptr, type->size_expr, ctx);

	auto *value = type->size_expr->accept(&m_comp_eval, {});

	if (auto *literal = dynamic_cast<LiteralExpr*>(value))
	{
		auto literal_value = literal->get_int();

		if (literal_value.has_value())
		{
			type->size = literal_value.value();

			return type;
		}
	}

	throw FrontendError{type->token, "Array size must be known at compile time"};
}

pars::Node * pars::Analyzer::visit(LiteralExpr *expr, VisitCtx ctx)
{
	return expr;
}

void pars::Analyzer::analyze(const std::vector<Node *> &nodes)
{
	visit_nodes(nodes);
}

pars::FnType * pars::Analyzer::get_current_fn()
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
		meta.mut_set = alias->meta.mut_set;
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

pars::Type * pars::Analyzer::get_type(std::string_view name, Token &error_token)
{
	auto *type = m_ctx->scope_table.find_symbol<Type>(name);

	if (type == nullptr)
	{
		throw FrontendError{error_token, fmt::format("unknown type name '{}'", name), nullptr};
	}

	return type;
}

pars::Expr* pars::Analyzer::visit_expr(Expr *parent, Expr *expr, VisitCtx ctx)
{
	ctx.depth = m_expr_depth++;

	auto *result = expr->accept(this, ctx);

	if (expr->mut_set.test(ctx.depth))
	{
		expr->flags |= ExprFlags::Immutable;
	}

	if (parent != nullptr)
	{
		parent->mut_set = expr->mut_set;
	}

	m_expr_depth--;

	return dynamic_cast<Expr*>(result);
}

pars::Analyzer::Analyzer(ParseCtx *parse_ctx)
{
	m_ctx = parse_ctx;
	m_comp_eval.parse_ctx = parse_ctx;
}

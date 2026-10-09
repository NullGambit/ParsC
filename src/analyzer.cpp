#include "analyzer.hpp"

#include <ranges>

#include "parse_ctx.hpp"
#include "ast.hpp"
#include "fn_collection.hpp"
#include "frontend_error.hpp"
#include "mangle.hpp"
#include "module.hpp"
#include "module_manager.hpp"
#include "util/fmt.hpp"

using enum pars::TokenType;

pars::Node* pars::Analyzer::visit(CallExpr *expr, VisitCtx ctx)
{
	ctx = new_ctx(expr, ctx);

	for (auto &arg : expr->arguments)
	{
		if (!arg->is_resolved() && !arg->is_ctx_sensitive())
		{
			arg = visit_expr(expr, arg, ctx);
		}
	}

	if (expr->callable->type == nullptr)
	{
		// set type from the expr in case of being called from a member access
		expr->callable->type = expr->type;

		expr->callable->accept(this, ctx);
	}

	if (expr->callable->type == nullptr)
	{
		throw FrontendError{expr->token, "could not resolve function overload"};
	}

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

	for (auto &arg : expr->arguments)
	{
		Type *ctx_type {};

		if (index < call_info.parameters.size())
		{
			ctx_type = call_info.parameters[index++]->type.ptr;
		}

		if (!arg->is_resolved())
		{
			arg = visit_expr(expr, arg, {ctx_type});
		}

		if (auto *named_param = EXPR_AS(arg, Named))
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
		auto *named = EXPR_AS(expr->arguments[position], Named);

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
			param->initializer = visit_expr(expr, param->initializer, {param->type.ptr});
		}
	}

	expr->type = call_info.return_meta.ptr;
	expr->mut_set = call_info.return_meta.mut_set;

	return expr;
}

pars::Node* pars::Analyzer::visit(FnType *fn, VisitCtx ctx)
{
	// already been analyzed
	if (has_flag(fn->flags, FnFlags::Resolved))
	{
		return fn;
	}

	auto scope = m_ctx->scope_table.new_scope();

	for (auto *param : fn->signature.parameters)
	{
		param->accept(this, {});
		param->type->accept(this, {});
	}

	if (!fn->symbol.name.empty())
	{
		if (!has_flag(fn->flags, FnFlags::Extern) && fn->symbol.name != "main")
		{
			Type *parent_type {};

			if (auto *impl = dynamic_cast<ImplStmt*>(ctx.invoker))
			{
				parent_type = impl->type;
			}

			mangle(fn->symbol.name, fn->signature.parameters, parent_type, fn->mangled_name, [](VarDeclStmt *param)
			{
				return param->type.ptr;
			});
		}

		if (fn->collection->fn_is_duplicate(fn))
		{
			throw FrontendError{fn->token, "Function is a duplicate"};
		}

		m_ctx->scope_table.add_to_scope(fn->symbol, fn, !has_flag(fn->flags, FnFlags::Private));
	}

	m_function_stack.emplace_back(fn);

	if (has_flag(fn->flags, FnFlags::ArrowFn))
	{
		auto *front = fn->body->nodes.front();

		if (front->base_kind == Node::BaseKind::Expr)
		{
			auto *expr = static_cast<Expr*>(front);

			// TODO resolve return type if manually typed
			expr->accept(this, {.type = fn->signature.return_type.ptr});

			fn->signature.return_type = expr->type;
		}
	}
	else
	{
		resolve_type(fn->signature.return_type, fn);

		if (fn->body != nullptr)
		{
			for (auto i = 0; auto *node : fn->body->nodes)
			{
				if (auto *expr = NODE_AS(node, Expr))
				{
					fn->body->nodes[i] = visit_expr(nullptr, expr, {});
				}
				else
				{
					node->accept(this, {});
				}

				i += 1;
			}
		}

		auto flags = m_ctx->scope_table.get_scope_data(m_ctx->scope_table.get_level()).flags;

		if (
			!fn->signature.return_type->is_equal(&VOID_TYPE)
			&&
			!has_flag(flags, ScopeFlags::HasReturn)
			&&
			fn->body != nullptr)
		{
			throw FrontendError{fn->token, "Not all paths return a value"};
		}
	}

	m_function_stack.pop_back();

	fn->flags |= FnFlags::Resolved;

	return fn;
}

pars::Node* pars::Analyzer::visit(StructType *stmt, VisitCtx ctx)
{
	// already been resolved
	if (has_flag(stmt->flags, StructFlags::Resolved))
	{
		return stmt;
	}

	for (auto &field : stmt->fields)
	{
		resolve_type(field.type, stmt);
	}

	stmt->flags |= StructFlags::Resolved;

	return stmt;
}

pars::Node* pars::Analyzer::visit(VarDeclStmt *stmt, VisitCtx ctx)
{
	// var x: T
	if (stmt->is_explicitly_typed())
	{
		resolve_type(stmt->type, stmt);
	}

	// var x = E
	if (stmt->initializer != nullptr)
	{
		stmt->initializer = visit_expr(nullptr, stmt->initializer, new_ctx(stmt, ctx, stmt->type.ptr));

		// var x = {}
		if (stmt->initializer->type == nullptr)
		{
			throw FrontendError{stmt->token, "Cannot infer type from initializer"};
		}

		if (!stmt->is_explicitly_typed())
		{
			stmt->type = stmt->initializer->type;
		}

		if (stmt->type.is_null())
		{
			auto top_level_mut = stmt->type.mut_set.test(0);

			stmt->type.mut_set = stmt->initializer->mut_set;

			if (top_level_mut)
			{
				stmt->type.mut_set.set(0);
			}
		}
	}

	if (auto *array = TYPE_AS(stmt->type.ptr, Array))
	{
		if (array->size == UNSIZED_ARRAY && stmt->initializer == nullptr)
		{
			throw FrontendError{stmt->token, "Cannot infer size of array"};
		}
		if (stmt->initializer != nullptr)
		{
			array->size = TYPE_AS(stmt->initializer->type, Array)->size;
		}
	}

	// var x: T = E
	if (stmt->initializer != nullptr && stmt->is_explicitly_typed() && !is_assignable_from(stmt->initializer->type, stmt->type.ptr))
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

		auto *result = stmt->initializer->accept(&m_comp_eval, {});

		stmt->initializer = NODE_AS(result, Expr);

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
		m_ctx->scope_table.add_import(stmt->module->source_file.id);
	}

	for (auto [import_name, symbol_name] : stmt->selective_imports)
	{
		auto *symbol = stmt->module->scope_table->find_local_symbol(import_name);

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

	stmt->expr->accept(this, {.type = fn->signature.return_type.ptr});

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

	ctx = new_ctx(stmt, ctx);

	for (auto i = 0; auto *node : stmt->nodes)
	{
		if (auto *expr = NODE_AS(node, Expr))
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

	auto *var = m_ctx->scope_table.find_symbol<VarDeclStmt>(stmt->lhs->get_symbol());

	if (var != nullptr)
	{
		var->flags |= VarFlags::Mutated;
	}

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

	if (!stmt->condition->type->is_equal(&BOOL_TYPE))
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
	stmt->stmt->accept(this, {});

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

		bind(last, const_cast<IntegerType*>(&I32_TYPE));
	}

	stmt->body->accept(this, {});

	return stmt;
}

pars::Node * pars::Analyzer::visit(ImplStmt *stmt, VisitCtx ctx)
{
	auto *symbol = m_ctx->scope_table.find_local_symbol(stmt->type_symbol.name);
	stmt->type  = dynamic_cast<UserDefType*>(symbol);

	if (stmt->type == nullptr)
	{
		throw FrontendError{stmt->token, "type does not support methods or is not in the same module as the impl statement"};
	}

	stmt->type->accept(this, {});

	stmt->type->impl = stmt;

	auto *self = new_node<PointerType>();

	self->set_inner(stmt->type);

	auto scope = m_ctx->scope_table.new_scope();

	m_ctx->scope_table.add_to_scope({.name = "Self"}, stmt->type);

	for (auto *method : stmt->methods)
	{
		VarDeclStmt *self_param {};

		if (!has_flag(method->flags, FnFlags::Static))
		{
			self_param = method->signature.parameters.front();
			self_param->type = self;
		}

		method->accept(this, {.invoker = stmt});

		if (self_param != nullptr && !has_flag(self_param->flags, VarFlags::Mutated))
		{
			self_param->type.mut_set.set(0);
		}
	}

	return stmt;
}

pars::Node* pars::Analyzer::visit(AliasType *alias, VisitCtx ctx)
{
	resolve_type(alias->type, alias);

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
		if (auto *enum_type = TYPE_AS(ctx.type, Enum))
		{
			auto *literal = enum_type->get_literal(expr->symbol);

			if (literal == nullptr)
			{
				throw FrontendError{expr->token,
					fmt::format("enum {} does not have a variant {}",
						enum_type->symbol.name,
						expr->symbol)};
			}

			return literal;
		}

		ScopeTable *table_override {};

		if (ctx.scope_table_override != nullptr)
		{
			table_override = ctx.scope_table_override;
		}

		auto *sym_node = find_symbol(expr->symbol, expr->token, table_override);
		auto *stmt_node = NODE_AS(sym_node, Stmt);

		if (auto *var = STMT_AS(stmt_node, VarDecl))
		{
			var->flags |= VarFlags::Used;
			expr->mut_set = var->type.mut_set;
			expr->type = var->type.ptr;
		}
		else if (auto *collection = dynamic_cast<FnCollection*>(sym_node))
		{
			auto *call = dynamic_cast<CallExpr*>(ctx.invoker);
			auto args = call != nullptr ? call->arguments : std::span<Expr*>{};

			expr->type = collection->get_fn(args, this);
		}
		else if (auto *type = NODE_AS(sym_node, Type))
		{
			expr->type = type;
		}
		else
		{
			throw FrontendError{expr->token, fmt::format("could not resolve the type of symbol {}", expr->symbol)};
		}

		expr->symbol_node = sym_node;
	}

	return expr;
}

pars::Node* pars::Analyzer::visit(BinaryExpr *expr, VisitCtx ctx)
{
	expr->left = visit_expr(expr, expr->left, new_ctx(expr, ctx, ctx.type));
	expr->right = visit_expr(expr, expr->right, {.type = expr->left->type});

	if (expr->op > _ComparisonStart && expr->op < _ComparisonEnd)
	{
		expr->type = const_cast<BoolType*>(&BOOL_TYPE);
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
	expr->right = visit_expr(expr, expr->right, new_ctx(expr, ctx));

	if (expr->op == '!')
	{
		expr->type = const_cast<BoolType*>(&BOOL_TYPE);
	}
	else
	{
		expr->type = expr->right->type;
	}

	return expr;
}

pars::Node* pars::Analyzer::visit(GroupExpr* expr, VisitCtx ctx)
{
	expr->inner = visit_expr(expr, expr->inner, new_ctx(expr, ctx));
	expr->type = expr->inner->type;

	return expr;
}

pars::Node* pars::Analyzer::visit(SizeofExpr* expr, VisitCtx ctx)
{
	expr->type = const_cast<IntegerType*>(&I32_TYPE);

	expr->expr = visit_expr(expr, expr->expr, new_ctx(expr, ctx));

	return expr;
}

pars::Node * pars::Analyzer::visit(NameofExpr *expr, VisitCtx ctx)
{
	expr->type = const_cast<StrType*>(&STR_TYPE);

	expr->expr = visit_expr(expr, expr->expr, new_ctx(expr, ctx));

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
	auto *stmt_node = NODE_AS(symbol_node, Stmt);
	auto *type_node = NODE_AS(symbol_node, Type);

	if (auto *import = STMT_AS(stmt_node, Import))
	{
		ctx.scope_table_override = import->module->scope_table;

		expr->accessor = visit_expr(expr, expr->accessor, new_ctx(expr, ctx));
	}
	else if (auto *enum_type = TYPE_AS(type_node, Enum))
	{
		auto *literal = enum_type->get_literal(expr->accessor->get_symbol());

		if (literal == nullptr)
		{
			throw FrontendError{expr->accessor->token,
				fmt::format("enum {} does not have a variant {}",
					enum_type->symbol.name,
					expr->accessor->get_symbol())};
		}

		return literal;
	}
	else if (type_node != nullptr && !expr->is_static_access)
	{
		auto *prop_expr = new_node<TypePropExpr>();

		auto *prop_symbol = EXPR_AS(expr->accessor, Symbol);

		if (prop_symbol == nullptr)
		{
			throw FrontendError{symbol_node->token, "Expected identifier for type property access"};
		}

		prop_expr->property_name = prop_symbol->symbol;

		prop_expr->type = type_node;

		expr->accessor = prop_expr;
		prop_expr->token = expr->token;
	}
	else
	{
		ctx = new_ctx(expr, ctx);

		if (expr->type == nullptr || ctx.member)
		{
			expr->target->type = expr->type;
			expr->target = visit_expr(expr, expr->target, ctx);
		}
		else
		{
			expr->target->type = expr->type;
		}

		auto subsymbol = expr->accessor->get_symbol();

		auto throw_error = [&] -> std::optional<MemberInfo>
		{
			throw FrontendError{expr->token, fmt::format("member '{}' does not exist on '{}' object",
				subsymbol, expr->target->get_symbol())};
		};

		auto *call = EXPR_AS(expr->accessor, Call);

		expr->is_method = call != nullptr;

		auto maybe_member = expr->is_method ? expr->target->type->get_method(subsymbol) : expr->target->type->get_member(subsymbol);

		// maybe its not a method and its just a function pointer field
		if (expr->is_method && !maybe_member.has_value())
		{
			maybe_member = expr->target->type->get_member(subsymbol);
			expr->is_method = false;
		}

		auto member = maybe_member.or_else(throw_error).value();

		if (expr->is_method && !expr->is_static_access)
		{
			auto *self_arg = expr->target;

			if (!TYPE_AS(expr->target->type, Pointer))
			{
				expr->target->flags |= ExprFlags::AlwaysPtr;

				auto *self = new_node<PointerType>();

				self->set_inner(expr->target->type->get_real_type());

				self_arg->type = self;
			}
			else
			{
				expr->target->flags |= ExprFlags::AlwaysLoad;
			}

			call->arguments.insert(call->arguments.begin(), self_arg);
		}

		if (call != nullptr)
		{
			call->callable->type = member.type;
		}

		expr->accessor->type = member.type;
		ctx.member = true;

		expr->accessor = visit_expr(expr, expr->accessor, ctx);
	}

	expr->type = expr->accessor->type;
	expr->mut_set = expr->target->mut_set;

	return expr;
}

pars::Node* pars::Analyzer::visit(CastExpr* expr, VisitCtx ctx)
{
	ctx = new_ctx(expr, ctx);

	resolve_type(expr->cast_type, expr);

	expr->type = expr->cast_type.ptr;
	expr->mut_set = expr->cast_type.mut_set;

	ctx.type = expr->type;

	expr->target = visit_expr(expr, expr->target, ctx);

	expr->original_type = expr->target->type;
	expr->target->type = expr->type;

	return expr;
}

pars::Node* pars::Analyzer::visit(NamedExpr *expr, VisitCtx ctx)
{
	expr->value = visit_expr(expr, expr->value, new_ctx(expr, ctx, ctx.type));
	expr->type = expr->value->type;

	return expr;
}

pars::Node* pars::Analyzer::visit(AbsExpr *expr, VisitCtx ctx)
{
	expr->value = visit_expr(expr, expr->value, new_ctx(expr, ctx));
	expr->type = expr->value->type;

	return expr;
}

pars::Node* pars::Analyzer::visit(PtrOpExpr *expr, VisitCtx ctx)
{
	expr->target = visit_expr(expr, expr->target, new_ctx(expr, ctx));

	expr->mut_set = expr->target->mut_set;

	switch (expr->op)
	{
		case Caret:
		{
			// in case target is actually a type this might be a pointer type.
			// useful in cases of nameof expressions.
			if (auto *symbol = EXPR_AS(expr->target, Symbol);
				auto *type = NODE_AS(symbol->symbol_node, Type))
			{
				auto *ptr = new_node<PointerType>();

				ptr->set_inner(type);

				symbol->type = ptr;

				return symbol;
			}

			if (!expr->target->type->is_ptr())
			{
				throw FrontendError{expr->token, "Dereference target is not a pointer"};
			}

			expr->type = expr->target->type->get_inner();

			break;
		}
		case Ampersand:
		{
			auto *p = new_node<PointerType>();

			p->set_inner(expr->target->type);

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

			if (auto *named_expr = EXPR_AS(initializer, Named))
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

	void assign_struct_indices(StructType *type, Analyzer *analyzer, InitializerList &initializers)
	{
		auto find_fn = [](const StructFieldInfo &field, NamedExpr *named_expr)
		{
			// TODO perhaps implementing a string interning system within the compiler to speed up this comparison
			// or see if llvm has one i can already use
			// or use hashes
			return named_expr->name == field.symbol.name;
		};

		auto get_type_fn = [](const StructFieldInfo &field) { return field.type.ptr; };

		assign_named_indices(type->fields, find_fn, get_type_fn, analyzer, initializers);
	}
}

pars::Node* pars::Analyzer::visit(ArrayLiteralExpr *expr, VisitCtx ctx)
{
	if (expr->initializers.empty())
	{
		return expr;
	}

	ctx = new_ctx(expr, ctx, ctx.type);

	auto *array_type = new_node<ArrayType>();

	if (expr->type_specifier != nullptr)
	{
		auto *type = get_type(expr->type_specifier->get_symbol(), expr->type_specifier->token);

		array_type->set_inner(type);

		auto *real = TYPE_AS(array_type->get_inner(), Array);

		if (real != nullptr)
		{
			array_type = real;
		}
	}
	else if (auto *array_ctx = dynamic_cast<BaseArrayType*>(produce_type(ctx.type)))
	{
		array_type->set_inner(array_ctx->get_inner());
	}

	expr->type = array_type;

	if (!array_type->members.empty())
	{
		auto find_fn = [](const std::string_view &name, NamedExpr *named_expr)
		{
			return named_expr->name == name;
		};

		auto get_type_fn = [array_type](const std::string_view &name) { return array_type->get_inner(); };

		assign_named_indices(array_type->members, find_fn, get_type_fn, this, expr->initializers);

		return expr;
	}

	array_type->size = expr->initializers.size();

	for (auto i = 0; auto &[element, pos] : expr->initializers)
	{
		element = visit_expr(expr, element, {.type = array_type->get_inner()});

		if (array_type->get_inner() == nullptr)
		{
			array_type->set_inner(element->type);
		}

		if (!array_type->get_inner()->is_equal(element->type))
		{
			throw FrontendError{element->token, "array literal element types dont all match"};
		}

		pos = i;
		i += 1;
	}

	if (auto *ctx_array = TYPE_AS(ctx.type, Array); ctx_array && ctx_array->size != UNSIZED_ARRAY)
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
	auto *left_symbol = find_symbol(expr->lhs->get_symbol(), expr->lhs->token, nullptr, /*permissive=*/true);

	if (NODE_AS(left_symbol, Type))
	{
		auto *literal = new_node<ArrayLiteralExpr>();

		literal->type_specifier = expr->lhs;
		literal->initializers = InitializerList{InitializerElement{expr->index}};

		return visit_expr(expr, literal, {});
	}

	// incase this came from somewhere like a member access
	expr->lhs->type = expr->type;
	expr->lhs = visit_expr(expr, expr->lhs, new_ctx(expr, ctx));
	expr->index = visit_expr(expr, expr->index, {});

	if (!expr->index->type->is_int())
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

	auto *struct_type = TYPE_AS(expr->type, Struct);

	assign_struct_indices(struct_type, this, expr->initializers);

	return expr;
}

pars::Node* pars::Analyzer::visit(AnonInitExpr *expr, VisitCtx ctx)
{
	if (ctx.type == nullptr && !expr->initializers.empty())
	{
		auto *type = new_node<StructType>();

		expr->type = type;

		for (auto i = 0; auto &[expr, pos] : expr->initializers)
		{
			if (auto *named = EXPR_AS(expr, Named))
			{
				named->value->accept(this, {});

				type->fields.emplace_back(
					StructFieldInfo
					{
						{named->name},
						named->value->type
					});
			}
			else
			{
				throw FrontendError{expr->token, "anon struct can only contain named fields"};
			}

			pos = i;
			i++;
		}

		return expr;
	}

	expr->type = produce_type(ctx.type);

	if (auto *struct_type = TYPE_AS(expr->type, Struct))
	{
		assign_struct_indices(struct_type, this, expr->initializers);
	}

	return expr;
}

pars::Node* pars::Analyzer::visit(SliceExpr *expr, VisitCtx ctx)
{
	expr->lhs = visit_expr(expr, expr->lhs, {});

	if (expr->start != nullptr)
	{
		expr->start = visit_expr(expr, expr->start, {});
	}

	if (expr->end != nullptr)
	{
		expr->end = visit_expr(expr, expr->end, {});
	}

	if (!expr->lhs->type->is_array())
	{
		throw FrontendError{expr->token, fmt::format("type of {} cannot be sliced", expr->type->get_type_name())};
	}

	auto *slice_type = new_node<SliceType>();

	slice_type->set_inner(expr->lhs->type->get_inner());

	expr->type = slice_type;

	return expr;
}

pars::Node* pars::Analyzer::visit(UnresolvedSymbolType *type, VisitCtx ctx)
{
	*ctx.result = get_type(type->symbol, type->token);

	return type;
}

pars::Node* pars::Analyzer::visit(PointerType *type, VisitCtx ctx)
{
	auto &inner = type->get_inner_ref();

	type->get_inner()->accept(this, {.result = (Node**)&inner});

	type->set_inner(inner);

	return type;
}

pars::Node* pars::Analyzer::visit(BaseArrayType *type, VisitCtx ctx)
{
	auto &inner = type->get_inner_ref();

	type->get_inner()->accept(this, {.result = (Node**)&inner});

	type->set_inner(inner);

	return type;
}

pars::Node* pars::Analyzer::visit(ArrayType *type, VisitCtx ctx)
{
	auto &inner = type->get_inner_ref();

	type->get_inner()->accept(this, {.result = (Node**)&inner});

	type->size_expr = visit_expr(nullptr, type->size_expr, {});

	Expr *value {};

	if (type->size_expr == nullptr)
	{
		goto ret;
	}

	value = NODE_AS(type->size_expr->accept(&m_comp_eval, {}), Expr);

	if (auto *literal = EXPR_AS(value, Literal); auto literal_value = literal->get_int())
	{
		if (literal_value.has_value())
		{
			type->size = literal_value.value();

			goto ret;
		}
	}

	throw FrontendError{type->token, "Array size must be known at compile time"};

	// goto haters seethe!!
ret:
	type->set_inner(inner);
	return type;
}

pars::Node * pars::Analyzer::visit(LiteralExpr *expr, VisitCtx ctx)
{
	expr->flags |= ExprFlags::ResolvedInternal;

	if (ctx.type != nullptr && expr->type->can_coerce_into(ctx.type))
	{
		expr->type = ctx.type;
	}

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

void pars::Analyzer::resolve_type(TypeMeta &meta, Node *node)
{
	if (meta.ptr != nullptr)
	{
		meta.ptr->accept(this, {.result = (Node**)&meta.ptr});
	}

	if (auto *alias = dynamic_cast<AliasType*>(meta.ptr))
	{
		meta.mut_set = alias->type.mut_set;
	}
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

pars::Node* pars::Analyzer::find_symbol(std::string_view name, Token &error_token, ScopeTable *table_override, bool permissive)
{
	auto *scope_table = table_override != nullptr ? table_override : &m_ctx->scope_table;
	auto *symbol = scope_table->find_symbol(name);

	if (symbol == nullptr && !permissive)
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
	if (expr == nullptr)
	{
		return nullptr;
	}

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

	return NODE_AS(result, Expr);
}

pars::Analyzer::Analyzer(ParseCtx *parse_ctx)
{
	m_ctx = parse_ctx;
	m_comp_eval.parse_ctx = parse_ctx;
}

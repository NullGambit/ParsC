#include "analyzer.hpp"

#include "parse_ctx.hpp"
#include "ast.hpp"
#include "frontend_error.hpp"
#include "module.hpp"
#include "module_manager.hpp"
#include "util/fmt.hpp"

using enum pars::TokenType;

void pars::Analyzer::visit(CallExpr *expr)
{
	for (auto *arg : expr->arguments)
	{
		arg->accept(this);
	}

	expr->prototype = find_symbol<FnDecl>(expr->symbol, expr->token);

	expr->prototype->accept(this);

	expr->symbol = expr->prototype->symbol.name;
	expr->type = expr->prototype->signature.return_type;
}

void pars::Analyzer::visit(FnDecl *fn)
{
	if (fn->signature.return_type != nullptr)
	{
		return;
	}

	for (auto *param : fn->signature.parameters)
	{
		param->type = resolve_type(param->type_name, param);
	}

	m_ctx->scope_table.add_to_scope(fn->symbol, fn);

	auto scope = m_ctx->scope_table.new_scope();

	for (auto *param : fn->signature.parameters)
	{
		m_ctx->scope_table.add_to_scope(param->symbol, param, PRIVATE_SYMBOL);
	}

	m_function_stack.emplace_back(fn);

	if (has_flag(fn->flags, FnFlags::ArrowFn))
	{
		auto *expr = dynamic_cast<Expr*>(fn->body.front());

		expr->accept(this);

		fn->signature.return_type = expr->type;
	}
	else
	{
		fn->signature.return_type = resolve_type(fn->signature.return_type_name, fn);

		for (auto *node : fn->body)
		{
			node->accept(this);
		}
	}

	m_function_stack.pop_back();
}

void pars::Analyzer::visit(VarDeclStmt *stmt)
{
	// var x: T
	if (!stmt->type_name.empty())
	{
		stmt->type = resolve_type(stmt->type_name, stmt);
	}

	// var x = E
	if (stmt->initializer != nullptr)
	{
		stmt->initializer->accept(this);

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

void pars::Analyzer::visit(ImportStmt *stmt)
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

void pars::Analyzer::visit(ReturnStmt *stmt)
{
	stmt->expr->accept(this);

	auto *fn = get_current_fn();

	if (!fn->signature.return_type->is_equal(stmt->expr->type))
	{
		throw FrontendError{stmt->token, fmt::format("Expected {} in return statement but got {}",
			fn->signature.return_type->get_type_name(), stmt->expr->type->get_type_name())};
	}
}

void pars::Analyzer::visit(AliasType *alias)
{
	alias->type = resolve_type(dynamic_cast<PendingType*>(alias->type)->symbol, alias);

	// TODO handle private aliasing
	m_ctx->scope_table.add_to_scope(alias->symbol, alias);
}

void pars::Analyzer::visit(SymbolExpr *expr)
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

void pars::Analyzer::visit(BinaryExpr *expr)
{
	expr->left->accept(this);
	expr->right->accept(this);

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

void pars::Analyzer::visit(UnaryExpr *expr)
{
	expr->right->accept(this);

	if (expr->op == '!')
	{
		expr->type = const_cast<Bool*>(&BoolType);
	}
	else
	{
		expr->type = expr->right->type;
	}
}

void pars::Analyzer::visit(GroupExpr* expr)
{
	expr->inner->accept(this);
	expr->type = expr->inner->type;
}

void pars::Analyzer::visit(SizeofExpr* expr)
{
	expr->expr->accept(this);
}

void pars::Analyzer::visit(MemberAccessExpr* expr)
{
	auto *symbol = find_symbol(expr->target_symbol, expr->token);

	if (auto *import = dynamic_cast<ImportStmt*>(symbol))
	{
		auto *old_ctx = m_ctx;

		m_ctx = import->module->ast.get_ctx();

		expr->accessor->accept(this);

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
		expr->accessor->accept(this);
	}

	expr->type = expr->accessor->type;
}

void pars::Analyzer::visit(CastExpr* expr)
{
	expr->type_expr->accept(this);

	expr->type = expr->type_expr->type;

	expr->target->accept(this);
	expr->original_type = expr->target->type;
	expr->target->type = expr->type;
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

pars::Type * pars::Analyzer::resolve_type(std::string_view name, Node *node)
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

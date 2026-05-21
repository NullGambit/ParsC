#include "resolver.hpp"

#include <ranges>

#include "expr.hpp"
#include "frontend_error.hpp"
#include "parse_ctx.hpp"
#include "stmt.hpp"
#include "util/fmt.hpp"

pars::Type* pars::resolve_type(ScopeTable &scope_table, std::string_view symbol, Token &error_token)
{
	auto *type = scope_table.find_symbol<Type>(symbol);

	if (type == nullptr)
	{
		throw FrontendError{error_token, fmt::format("unknown type name '{}'", symbol), nullptr};
	}

	return type;
}

void pars::Resolver::visit(FnDecl *fn)
{
	if (has_flag(fn->flags, FnFlags::ArrowFn))
	{
		fn->signature.return_type = dynamic_cast<Expr*>(fn->body.front())->type;
	}
	else
	{
		fn->signature.return_type = resolve_type(m_ctx->scope_table, fn->signature.return_type_name, fn->token);
	}

	for (auto *param : fn->signature.parameters)
	{
		param->type = resolve_type(m_ctx->scope_table, param->type_name, param->token);
	}
}

void pars::Resolver::resolve(const std::vector<Node *> &nodes)
{
	auto &scope = m_ctx->scope_table.get_scope();

	for (auto node : scope | std::views::values)
	{
		node->accept(this);
	}
}

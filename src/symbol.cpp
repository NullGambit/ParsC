#include "symbol.hpp"

#include "expr.hpp"

static std::vector<pars::Expr*> g_attributes;

u32 pars::set_attributes(std::vector<Expr*> &attributes)
{
	const auto n = g_attributes.size();
	const auto id = n == 0 ? n : n - 1;

	g_attributes.insert(g_attributes.begin(), attributes.begin(), attributes.end());

	return id;
}

std::span<pars::Expr *> pars::get_attributes(Symbol symbol)
{
	return std::span{g_attributes.begin() + symbol.attribute_id, symbol.attribute_count};
}

bool pars::has_keyword_attribute(Symbol symbol, TokenType type)
{
	auto attributes = get_attributes(symbol);

	return std::ranges::any_of(attributes.begin(), attributes.end(),
		[type](Expr const *expr)
		{
			return expr != nullptr && expr->token.type == type;
		});
}


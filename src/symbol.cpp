#include "symbol.hpp"

#include "expr.hpp"

static std::vector<pars::TokenType> g_attributes;

u32 pars::set_attributes(std::vector<TokenType> &attributes)
{
	const auto n = g_attributes.size();

	g_attributes.insert(g_attributes.begin() + g_attributes.size(), attributes.begin(), attributes.end());

	return n;
}

std::span<pars::TokenType> pars::get_attributes(Symbol symbol)
{
	if (symbol.attribute_id == NO_ATTRIBUTES)
	{
		return {};
	}

	return std::span{g_attributes.begin() + symbol.attribute_id, symbol.attribute_count};
}

bool pars::has_keyword_attribute(Symbol symbol, TokenType type)
{
	auto attributes = get_attributes(symbol);

	return std::ranges::any_of(attributes.begin(), attributes.end(),
		[type](TokenType this_type)
		{
			return this_type == type;
		});
}


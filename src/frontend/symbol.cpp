#include "symbol.hpp"

#include "expr.hpp"

static std::vector<pars::Expr*> g_attributes;

void pars::set_attributes(std::vector<Expr*> &attributes)
{
	g_attributes.insert(g_attributes.begin(), attributes.begin(), attributes.end());
}

std::span<pars::Expr *> pars::get_attributes(Symbol symbol)
{
	return std::span{g_attributes.begin() + symbol.attribute_id, symbol.attribute_count};
}

size_t pars::get_attribute_id()
{
	auto size = g_attributes.size();
	return  size == 0 ? 0 : size + 1;
}

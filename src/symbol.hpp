#pragma once

#include <span>
#include <vector>

#include "node.hpp"
#include "visitor.hpp"

namespace pars
{
	struct Expr;

	constexpr auto NO_ATTRIBUTES = UINT32_MAX;

	struct Symbol
	{
		std::string_view name;
		u32 attribute_id = NO_ATTRIBUTES;
		u8 attribute_count;
	};

	void set_attributes(std::vector<Expr*> &attributes);

	std::span<Expr*> get_attributes(Symbol symbol);

	size_t get_attribute_id();
}

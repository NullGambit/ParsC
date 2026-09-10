#pragma once

#include <bitset>

namespace pars
{
	struct Type;

	/*
	 *	{x: i32, y: str}
	 */

	using MutSet = std::bitset<32>;

	struct TypeMeta
	{
		MutSet mut_set;
		Type *type {};
	};
}

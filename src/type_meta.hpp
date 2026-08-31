#pragma once

#include <bitset>

namespace pars
{
	struct Type;

	/*
	 *	{x: i32, y: str}
	 */

	using ConstSet = std::bitset<32>;

	struct TypeMeta
	{
		ConstSet mut_set;
		Type *type {};
	};
}

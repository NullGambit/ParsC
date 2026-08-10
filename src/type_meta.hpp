#pragma once

#include <bitset>

namespace pars
{
	struct Type;

	/*
	 *	[3]i32{"x", "y", "z"}
	 *	{x: i32, y: str}
	 *	fn(i32, str): bool
	 *	[3]fn(i32, str): bool
	 */

	using ConstSet = std::bitset<32>;

	struct TypeMeta
	{
		ConstSet const_set;
		Type *type {};
	};
}

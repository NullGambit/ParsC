#pragma once

#include <bitset>

namespace pars
{
	struct Type;

	/*
	 *	{x: i32, y: str}
	 *	fn(i32, str): bool
	 *	[3]fn(i32, str): bool
	 *	fn_ptrs[0](10, "hello)
	 */

	using ConstSet = std::bitset<32>;

	struct TypeMeta
	{
		ConstSet mut_set;
		Type *type {};
	};
}

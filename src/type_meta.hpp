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
	 *	const []i32
	 *	[]const i32
	 *	const []const i32
	 *	^const i32
	 *	const^i32
	 *	const^const i32
	 *
	 *	var x: ^const i32 = do_stuff()
	 */

	using ConstSet = std::bitset<32>;

	struct TypeMeta
	{
		ConstSet const_set;
		Type *type {};
	};
}

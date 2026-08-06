#pragma once
#include <bitset>
#include <string_view>
#include <cstddef>

#include "util/macros.hpp"

namespace pars
{
	/*
	 *	i32
	 *	^i32
	 *	^^i32
	 *	[]i32
	 *	[?]i32
	 *	[3]i32
	 *	[3]i32{"x", "y", "z"}
	 *	[3][3]i32
	 *	{x: i32, y: str}
	 *	fn(i32, str): bool
	 *	[3]fn(i32, str): bool
	 *	const []i32
	 *	[]const i32
	 *	const []const i32
	 *	^const i32
	 *	const^i32
	 *	const^const i32
	 */

	struct TypeMeta
	{
		std::bitset<32> const_set;
		Type *type {};
	};
}

#pragma once
#include <string_view>

#include "util/macros.hpp"

namespace pars
{
	enum class TypeFlags
	{
		Pointer = 1 << 0,
		Const = 1 << 1,
		OuterConst = 1 << 2,
		InnerConst = 1 << 3,
		Array = 1 << 4,
	};

	PARS_FLAGIFY(TypeFlags);

	constexpr auto UNSIZED_ARRAY = UINT32_MAX;

	struct TypeMeta
	{
		std::string_view name;
		TypeFlags flags {};
		u32 array_size {};
	};
}

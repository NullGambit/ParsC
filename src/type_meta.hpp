#pragma once
#include <string_view>
#include <cstddef>

#include "util/macros.hpp"

namespace pars
{
	enum class TypeFlags
	{
		Pointer = 1 << 0,
		Const = 1 << 1,
		Array = 1 << 2,
		ArrayInferSize =  1 << 3,
	};

	PARS_FLAGIFY(TypeFlags);

	constexpr auto UNSIZED_ARRAY = UINT32_MAX;

	struct TypeMeta
	{
		std::string_view name;
		TypeFlags flags {};
		u32 array_size = UNSIZED_ARRAY;

	};
}

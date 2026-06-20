#pragma once
#include <string_view>

#include "util/macros.hpp"

namespace pars
{
	enum class TypeFlags
	{
		Pointer = 1 << 0,
		Const = 1 << 1,
	};

	PARS_FLAGIFY(TypeFlags);

	struct TypeMeta
	{
		std::string_view name;
		TypeFlags flags {};
	};
}

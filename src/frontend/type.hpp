#pragma once
#include <variant>

#include "node.hpp"

namespace pars
{
	struct Type : Node
	{
		virtual bool is_equal(Type *other) = 0;
	};

	struct PrimitiveType : Type
	{
		enum class Kind
		{
			I32,
			F32,
			Bool,
			Char,
			Str,
		} kind {};

		bool is_equal(Type *other) override;
	};
}

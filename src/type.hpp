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
		enum class Kind : u8
		{
			Void,
			I32,
			F32,
			Bool,
			Char,
		} kind {};

		bool is_equal(Type *other) override;
	};
}

#pragma once

#include <bitset>

namespace pars
{
	struct Type;

	using MutSet = std::bitset<32>;

	// stores a type packed with information such as mutability
	struct TypeMeta
	{
		MutSet mut_set;
		Type *ptr {};

		TypeMeta() = default;

		TypeMeta(Type *ptr) :
			ptr{ptr}
		{}

		Type* operator->() const
		{
			return ptr;
		}

		[[nodiscard]]
		bool is_valid() const
		{
			return ptr != nullptr;
		}

		[[nodiscard]]
		bool is_null() const
		{
			return ptr == nullptr;
		}
	};
}

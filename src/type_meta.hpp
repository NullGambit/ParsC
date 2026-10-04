#pragma once

#include <bitset>

namespace pars
{
	struct Type;

	using MutSet = std::bitset<32>;

	Type* produce_type(Type const *type);

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

		template<class T>
		T* produce()
		{
			return dynamic_cast<T*>(produce_type(ptr));
		}
	};
}

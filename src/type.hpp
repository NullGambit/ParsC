#pragma once
#include <variant>

#include "node.hpp"

namespace llvm
{
	class LLVMContext;
	class Type;
}

namespace pars
{
	constexpr auto IS_SIGNED = true;

	struct Type : Node
	{
		virtual bool is_equal(Type const *other) const = 0;
		virtual llvm::Type* get_llvm_type(llvm::LLVMContext *ctx) const = 0;
		virtual bool is_primitive() const { return false; }
	};

	template<class T>
	T const * types_match(Type const *a_type, Type  const *b_type)
	{
		auto *a = dynamic_cast<const T*>(a_type);
		auto *b = dynamic_cast<const T*>(b_type);

		if (a != nullptr && b != nullptr)
		{
			return b;
		}

		return nullptr;
	}

#define DEFAULT_TYPE_EQUAL(T) bool is_equal(Type const *other) const override { return types_match<Void>(this, other); }

#define DEFAULT_INTEGRAL_EQUAL(T)												\
virtual bool is_equal(Type const *other) const override							\
{																				\
	auto other_int = types_match<T>(this, other);								\
																				\
	if (other_int != nullptr)													\
	{																			\
		return bits == other_int->bits && is_signed == other_int->is_signed;	\
	}																			\
																				\
	return false;																\
}																				\

	struct Void : Type
	{
		DEFAULT_TYPE_EQUAL(Void)

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;

		bool is_primitive() const override
		{
			return true;
		}
	};

	struct Integral : Type
	{
		u8 bits;
		bool is_signed;

		Integral(u8 bits, bool is_signed) :
			bits{bits},
			is_signed{is_signed}
		{}

		bool is_primitive() const final
		{
			return true;
		}

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;

		DEFAULT_INTEGRAL_EQUAL(Integral)
	};

	struct Integer : Integral
	{
		Integer(u8 bits, bool is_signed) :
			Integral{bits, is_signed}
		{}

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;

		DEFAULT_INTEGRAL_EQUAL(Integer)
	};

	struct Float : Integral
	{
		Float(u8 bits, bool is_signed) :
			Integral{bits, is_signed}
		{}

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;

		DEFAULT_INTEGRAL_EQUAL(Float)
	};

	struct Bool : Integral
	{
		Bool(u8 bits, bool is_signed) :
			Integral{bits, is_signed}
		{}

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;

		DEFAULT_INTEGRAL_EQUAL(Bool)
	};

	struct Char : Integral
	{
		Char(u8 bits, bool is_signed) :
			Integral{bits, is_signed}
		{}

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;

		DEFAULT_INTEGRAL_EQUAL(Char)
	};

	static const Void VoidType;
	static const Integer I8Type {8, IS_SIGNED};
	static const Integer U8Type {8, !IS_SIGNED};
	static const Integer I16Type {16, IS_SIGNED};
	static const Integer U16Type {16, !IS_SIGNED};
	static const Integer I32Type {32, IS_SIGNED};
	static const Integer U32Type {32, !IS_SIGNED};
	static const Integer I64Type {64, IS_SIGNED};
	static const Integer U64Type {64, !IS_SIGNED};
	static const Float F32Type {32, IS_SIGNED};
	static const Float F64Type {64, IS_SIGNED};
	static const Bool BoolType {8, !IS_SIGNED};
	static const Char CharType {8, IS_SIGNED};
	static const Char UCharType {8, !IS_SIGNED};
}

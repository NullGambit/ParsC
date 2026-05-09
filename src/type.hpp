#pragma once
#include <variant>

#include "node.hpp"
#include "symbol.hpp"

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
		virtual std::string_view get_type_name() const = 0;
	};

	bool check_type_equality(Type const *a_type, Type  const *b_type);

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

#define DEFAULT_TYPE_EQUAL(T) bool is_equal(Type const *other) const override { return types_match<T>(this, other); }

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

		std::string_view get_type_name() const override
		{
			return "void";
		}
	};

	struct Integral : Type
	{
		u8 bits;
		bool is_signed;
		std::string_view type_name;

		Integral(u8 bits, bool is_signed, std::string_view type_name) :
			bits{bits},
			is_signed{is_signed},
			type_name{type_name}
		{}

		bool is_primitive() const final
		{
			return true;
		}

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;

		std::string_view get_type_name() const override
		{
			return type_name;
		}

		DEFAULT_INTEGRAL_EQUAL(Integral)
	};

	struct AliasType : Type
	{
		Symbol symbol;
		Type *type = nullptr;
		bool is_distinct = false;

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;
		std::string_view get_type_name() const override;
		bool is_equal(Type const *other) const override;

		ACCEPT
	};

	struct Integer : Integral
	{
		Integer(u8 bits, bool is_signed, std::string_view type_name) :
			Integral{bits, is_signed, type_name}
		{}

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;

		DEFAULT_INTEGRAL_EQUAL(Integer)
	};

	struct Float : Integral
	{
		Float(u8 bits, bool is_signed, std::string_view type_name) :
			Integral{bits, is_signed, type_name}
		{}

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;

		DEFAULT_INTEGRAL_EQUAL(Float)
	};

	struct Bool : Integral
	{
		Bool(u8 bits, bool is_signed, std::string_view type_name) :
			Integral{bits, is_signed, type_name}
		{}

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;

		DEFAULT_INTEGRAL_EQUAL(Bool)
	};

	struct Char : Integral
	{
		Char(u8 bits, bool is_signed, std::string_view type_name) :
			Integral{bits, is_signed, type_name}
		{}

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;

		DEFAULT_INTEGRAL_EQUAL(Char)
	};

	static const Void VoidType {};
	static const Integer I8Type {8, IS_SIGNED, "i8"};
	static const Integer U8Type {8, !IS_SIGNED, "u8"};
	static const Integer I16Type {16, IS_SIGNED, "i16"};
	static const Integer U16Type {16, !IS_SIGNED, "u16"};
	static const Integer I32Type {32, IS_SIGNED, "i32"};
	static const Integer U32Type {32, !IS_SIGNED, "u32"};
	static const Integer I64Type {64, IS_SIGNED, "i64"};
	static const Integer U64Type {64, !IS_SIGNED, "u64"};
	static const Float F32Type {32, IS_SIGNED, "f32"};
	static const Float F64Type {64, IS_SIGNED, "f64"};
	static const Bool BoolType {8, !IS_SIGNED, "bool"};
	static const Char CharType {8, IS_SIGNED, "char"};
	static const Char UCharType {8, !IS_SIGNED, "uchar"};

	struct Str : Type
	{
		DEFAULT_TYPE_EQUAL(Str)

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;

		std::string_view get_type_name() const override
		{
			return "str";
		}
	};

	static const Str StrType {};
}

#pragma once
#include <variant>

#include "node.hpp"
#include "symbol.hpp"
#include "type_meta.hpp"

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
		virtual bool is_primitive() const { return false; }

		virtual llvm::Type* get_llvm_type(llvm::LLVMContext *ctx) const = 0;
		virtual std::string_view get_type_name() const = 0;
		virtual u32 get_size() { return 1; }
		virtual llvm::Value* get_default_value(llvm::LLVMContext *ctx) const = 0;
		virtual llvm::Value* get_property(llvm::LLVMContext *ctx, std::string_view name);
		virtual Type* get_inner() const { return nullptr; }
		virtual bool is_ptr() const { return false; }
		virtual bool is_array() const { return false; }

		virtual llvm::Value* op_binary(EmitCtx &ctx, TokenType op, llvm::Value *lhs, llvm::Value *rhs) const { return nullptr; }
		virtual llvm::Value* op_unary(EmitCtx &ctx, TokenType op, llvm::Value *rhs) const { return nullptr; }
		virtual llvm::Value* op_in(EmitCtx &ctx, llvm::Value *lhs, llvm::Value *rhs) const { return nullptr; }
		virtual llvm::Value* op_abs(EmitCtx &ctx, llvm::Value *value) const { return nullptr; }
		virtual llvm::Value* op_cast(EmitCtx &ctx, llvm::Value *value, Type *desired_type) const { return nullptr; }
		virtual llvm::Value* op_coerce(EmitCtx &ctx, llvm::Value *value, Type *desired_type) const { return nullptr; }
		virtual llvm::Value* op_index(EmitCtx &ctx, llvm::Value *target, llvm::Value *index) const { return nullptr; }
		virtual bool can_coerce_into(Type const *desired_type) const { return false; }

		virtual bool is_iterable() const { return false; }
		virtual std::span<Type*> get_iter_bindings() const { return {}; }
		virtual llvm::Value* iter_emit_init(EmitCtx &ctx, Expr *iterable, std::span<llvm::Value*> vars) const { return nullptr; }
		virtual llvm::Value* iter_emit_update(EmitCtx &ctx, Expr *iterable, std::span<llvm::Value*> vars) const { return nullptr; }
		virtual llvm::Value* iter_emit_condition(EmitCtx &ctx, Expr *iterable, std::span<llvm::Value*> vars) const { return nullptr; }
	};

	bool check_type_equality(Type const *a_type, Type  const *b_type);

	bool is_assignable_from(Type const *from, Type  const *to);

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

		llvm::Value *get_default_value(llvm::LLVMContext *ctx) const override;
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

		llvm::Value *get_default_value(llvm::LLVMContext *ctx) const override;

		llvm::Value* get_property(llvm::LLVMContext* ctx, std::string_view name) override;

		u32 get_size() override;

		llvm::Value *op_cast(EmitCtx &ctx, llvm::Value *value, Type *desired_type) const override;
		llvm::Value *op_coerce(EmitCtx &ctx, llvm::Value *value, Type *desired_type) const override;
		bool can_coerce_into(Type const *desired_type) const override;

		DEFAULT_INTEGRAL_EQUAL(Integral)
	};

	struct AliasType : Type
	{
		Symbol symbol;
		TypeMeta meta;
		Type *type = nullptr;
		bool is_distinct = false;

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;
		std::string_view get_type_name() const override;
		bool is_equal(Type const *other) const override;

		llvm::Value *op_abs(EmitCtx &ctx, llvm::Value *value) const override;
		llvm::Value *op_binary(EmitCtx &ctx, TokenType op, llvm::Value *lhs, llvm::Value *rhs) const override;
		llvm::Value *op_cast(EmitCtx &ctx, llvm::Value *value, Type *desired_type) const override;
		llvm::Value *op_coerce(EmitCtx &ctx, llvm::Value *value, Type *desired_type) const override;
		llvm::Value *op_in(EmitCtx &ctx, llvm::Value *lhs, llvm::Value *rhs) const override;
		llvm::Value *op_unary(EmitCtx &ctx, TokenType op, llvm::Value *rhs) const override;
		llvm::Value *op_index(EmitCtx &ctx, llvm::Value *target, llvm::Value *index) const override;

		u32 get_size() override;

		llvm::Value *get_default_value(llvm::LLVMContext *ctx) const override;
		bool is_ptr() const override;
		bool is_array() const override;
		Type *get_inner() const override;

		ACCEPT
	};

	struct Integer : Integral
	{
		Integer(u8 bits, bool is_signed, std::string_view type_name) :
			Integral{bits, is_signed, type_name}
		{}

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;

		llvm::Value *op_binary(EmitCtx &ctx, TokenType op, llvm::Value *lhs, llvm::Value *rhs) const override;
		llvm::Value *op_unary(EmitCtx &ctx, TokenType op, llvm::Value *rhs) const override;

		llvm::Value *op_abs(EmitCtx &ctx, llvm::Value *value) const override;

		DEFAULT_INTEGRAL_EQUAL(Integer)
	};

	struct Float : Integral
	{
		Float(u8 bits, bool is_signed, std::string_view type_name) :
			Integral{bits, is_signed, type_name}
		{}

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;

		llvm::Value *op_binary(EmitCtx &ctx, TokenType op, llvm::Value *lhs, llvm::Value *rhs) const override;
		llvm::Value *op_unary(EmitCtx &ctx, TokenType op, llvm::Value *rhs) const override;
		llvm::Value *get_default_value(llvm::LLVMContext *ctx) const override;

		llvm::Value *op_abs(EmitCtx &ctx, llvm::Value *value) const override;

		DEFAULT_INTEGRAL_EQUAL(Float)
	};

	struct Bool : Integral
	{
		Bool(u8 bits, bool is_signed, std::string_view type_name) :
			Integral{bits, is_signed, type_name}
		{}

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;

		llvm::Value *op_binary(EmitCtx &ctx, TokenType op, llvm::Value *lhs, llvm::Value *rhs) const override;
		llvm::Value *op_unary(EmitCtx &ctx, TokenType op, llvm::Value *rhs) const override;

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

	struct Pointer : Integer
	{
		const Type *inner {};

		Pointer() :
			Integer{64, IS_SIGNED, "pointer"}
		{}

		explicit Pointer(const Type *inner) :
			Integer{64, IS_SIGNED, "pointer"},
			inner{inner}
		{}

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;

		std::string_view get_type_name() const override;

		llvm::Value *get_default_value(llvm::LLVMContext *ctx) const override;

		llvm::Value *op_binary(EmitCtx &ctx, TokenType op, llvm::Value *lhs, llvm::Value *rhs) const override;

		llvm::Value *op_abs(EmitCtx &ctx, llvm::Value *value) const override { return nullptr; }

		llvm::Value *op_coerce(EmitCtx &ctx, llvm::Value *value, Type *desired_type) const override
		{
			return value;
		}

		bool is_equal(Type const *other) const override;

		bool is_ptr() const override
		{
			return true;
		}
	};

	struct Packed : Type
	{
		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override
		{
			return nullptr;
		}

		std::string_view get_type_name() const override;

		llvm::Value *get_default_value(llvm::LLVMContext *ctx) const override;

		bool is_equal(Type const *other) const override
		{
			return true;
		}
	};

	struct Array : Type
	{
		Type *element_type;
		u32 size;

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;

		std::string_view get_type_name() const override;

		llvm::Value *get_default_value(llvm::LLVMContext *ctx) const override;

		bool is_equal(Type const *other) const override;

		llvm::Value *op_index(EmitCtx &ctx, llvm::Value *target, llvm::Value *index) const override;

		Type *get_inner() const override;

		bool is_array() const override
		{
			return true;
		}
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
	static const Bool BoolType {1, !IS_SIGNED, "bool"};
	static const Char CharType {8, IS_SIGNED, "char"};
	static const Char UCharType {8, !IS_SIGNED, "uchar"};
	static const Pointer VoidPointerType {&VoidType};

	struct Str : Pointer
	{
		DEFAULT_TYPE_EQUAL(Str)

		Str() :
			Pointer{&CharType}
		{}

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;

		std::string_view get_type_name() const override
		{
			return "str";
		}

		llvm::Value *get_default_value(llvm::LLVMContext *ctx) const override;
	};

	static const Str StrType {};

	struct RangeType : Type
	{
		DEFAULT_TYPE_EQUAL(RangeType)

		llvm::Type* get_llvm_type(llvm::LLVMContext *ctx) const override { return nullptr; }

		std::string_view get_type_name() const override { return "range"; }

		llvm::Value * get_default_value(llvm::LLVMContext *ctx) const override { return nullptr; }

		llvm::Value *op_in(EmitCtx &ctx, llvm::Value *lhs, llvm::Value *rhs) const override;

		bool is_iterable() const override
		{
			return true;
		}
		std::span<Type*> get_iter_bindings() const override;
		llvm::Value *iter_emit_init(EmitCtx &ctx, Expr *iterable, std::span<llvm::Value *> vars) const override;
		llvm::Value *iter_emit_condition(EmitCtx &ctx, Expr *iterable, std::span<llvm::Value *> vars) const override;
		llvm::Value *iter_emit_update(EmitCtx &ctx, Expr *iterable, std::span<llvm::Value *> vars) const override;
	};

	static const RangeType Range {};
}

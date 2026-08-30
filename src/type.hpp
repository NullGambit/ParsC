#pragma once
#include <optional>
#include <variant>

#include "node.hpp"
#include "symbol.hpp"
#include "type_meta.hpp"
#include "util/macros.hpp"

namespace llvm
{
	class LLVMContext;
	class Type;
}

namespace pars
{
	constexpr auto IS_SIGNED = true;

	enum class MemberAccess
	{
		Public,
		Private,
		Readonly,
	};

	struct MemberInfo
	{
		std::string_view name;
		Type *type;
		MemberAccess access;
	};

	struct CallInfo
	{
		std::span<VarDeclStmt*> parameters;
		TypeMeta return_meta;
		bool is_variadic {};
		u32 callable_arity {};
	};

	struct Type : Node
	{
		virtual bool is_equal(Type const *other) const = 0;
		virtual bool is_primitive() const { return false; }

		virtual llvm::Type* get_llvm_type(llvm::LLVMContext *ctx) const = 0;

		virtual llvm::Constant* get_aggregate_constant(EmitCtx &ctx, llvm::ArrayRef<llvm::Constant *> init_list) const
		{
			return nullptr;
		}

		virtual std::string_view get_type_name() const = 0;
		virtual u32 get_size() { return 1; }
		virtual llvm::Value* get_default_value(llvm::LLVMContext *ctx) const = 0;
		virtual llvm::Value* get_property(llvm::LLVMContext *ctx, std::string_view name);
		virtual Type* get_inner() const { return nullptr; }
		virtual bool is_ptr() const { return false; }
		virtual bool is_array() const { return false; }
		virtual bool is_struct() const { return false; }
		virtual bool is_callable() const { return false; }

		virtual llvm::Value* access_member(EmitCtx &ctx, llvm::Value *ptr, llvm::Value *accessor, std::string_view symbol) const { return nullptr; }
		virtual std::optional<MemberInfo> get_member(std::string_view symbol) const { return {}; }
		virtual std::optional<CallInfo> get_call_info() { return {}; }

		virtual llvm::Value* op_binary(EmitCtx &ctx, TokenType op, llvm::Value *lhs, llvm::Value *rhs) const { return nullptr; }
		virtual llvm::Value* op_unary(EmitCtx &ctx, TokenType op, llvm::Value *rhs) const { return nullptr; }
		virtual llvm::Value* op_in(EmitCtx &ctx, llvm::Value *lhs, llvm::Value *rhs) const { return nullptr; }
		virtual llvm::Value* op_abs(EmitCtx &ctx, llvm::Value *value) const { return nullptr; }
		virtual llvm::Value* op_cast(EmitCtx &ctx, llvm::Value *value, Type *desired_type) const { return nullptr; }
		virtual llvm::Value* op_coerce(EmitCtx &ctx, llvm::Value *value, Type *desired_type) const { return nullptr; }
		virtual llvm::Value* op_index(EmitCtx &ctx, llvm::Value *target, llvm::Value *index) const { return nullptr; }
		virtual llvm::Value* op_slice(EmitCtx &ctx, llvm::Value *array, llvm::Value *target, llvm::Value *start, llvm::Value *end) const { return nullptr; }
		// llvm doesnt seem to like std::span
		virtual llvm::Value* op_call(EmitCtx &ctx, llvm::Value *callable, llvm::ArrayRef<llvm::Value*> args) const { return nullptr; }
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

	// represents a symbol that has not been resolved yet.
	// should not get past the frontend.
	struct UnresolvedSymbol : Type
	{
		std::string_view symbol;

		bool is_equal(Type const *other) const override { return false; }

		llvm::Type * get_llvm_type(llvm::LLVMContext *ctx) const override { return nullptr; }

		std::string_view get_type_name() const override { return "unresolved"; }

		llvm::Value * get_default_value(llvm::LLVMContext *ctx) const override { return nullptr; }

		ACCEPT
	};

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
		llvm::Value *op_call(EmitCtx &ctx, llvm::Value *callable, llvm::ArrayRef<llvm::Value *> args) const override;

		u32 get_size() override;

		llvm::Value *get_default_value(llvm::LLVMContext *ctx) const override;
		std::optional<CallInfo> get_call_info() override;
		bool is_ptr() const override;
		bool is_array() const override;
		bool is_struct() const override;
		Type *get_inner() const override;
		bool is_callable() const override;

		std::optional<MemberInfo> get_member(std::string_view symbol) const override;
		llvm::Value *access_member(EmitCtx &ctx, llvm::Value *ptr, llvm::Value *accessor, std::string_view symbol) const override;

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

		llvm::Value *op_binary(EmitCtx &ctx, TokenType op, llvm::Value *lhs, llvm::Value *rhs) const override;

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;

		DEFAULT_INTEGRAL_EQUAL(Char)
	};

	struct Pointer : Integer
	{
		Type *inner {};

		ACCEPT

		Pointer() :
			Integer{64, IS_SIGNED, "pointer"}
		{}

		explicit Pointer(Type *inner) :
			Integer{64, IS_SIGNED, "pointer"},
			inner{inner}
		{}

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;

		std::string_view get_type_name() const override;

		std::optional<MemberInfo> get_member(std::string_view symbol) const override;
		llvm::Value *access_member(EmitCtx &ctx, llvm::Value *ptr, llvm::Value *accessor, std::string_view symbol) const override;

		llvm::Value *get_default_value(llvm::LLVMContext *ctx) const override;

		llvm::Value *op_binary(EmitCtx &ctx, TokenType op, llvm::Value *lhs, llvm::Value *rhs) const override;

		llvm::Value *op_abs(EmitCtx &ctx, llvm::Value *value) const override { return nullptr; }

		llvm::Value *op_coerce(EmitCtx &ctx, llvm::Value *value, Type *desired_type) const override
		{
			return value;
		}

		bool can_coerce_into(Type const *desired_type) const override
		{
			return false;
		}

		Type *get_inner() const override
		{
			return inner;
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

	struct BaseArray : Type
	{
		Type *element_type;

		ACCEPT

		llvm::Value *get_default_value(llvm::LLVMContext *ctx) const override;

		llvm::Value *op_index(EmitCtx &ctx, llvm::Value *target, llvm::Value *index) const override;

		std::optional<MemberInfo> get_member(std::string_view symbol) const override;

		Type *get_inner() const override;

		bool is_array() const override
		{
			return true;
		}

		llvm::Value *do_op_index(EmitCtx &ctx, llvm::Value *target, llvm::Value *index, llvm::Type *type) const;

		bool is_iterable() const override
		{
			return true;
		}

		std::span<Type*> get_iter_bindings() const override;
		llvm::Value *iter_emit_init(EmitCtx &ctx, Expr *iterable, std::span<llvm::Value *> vars) const override;
		llvm::Value *iter_emit_condition(EmitCtx &ctx, Expr *iterable, std::span<llvm::Value *> vars) const override;
		llvm::Value *iter_emit_update(EmitCtx &ctx, Expr *iterable, std::span<llvm::Value *> vars) const override;
	};

	constexpr auto UNSIZED_ARRAY = UINT32_MAX;

	struct Array : BaseArray
	{
		std::vector<std::string_view> members;
		// a temporary expression that must compile to a constant and will be assigned to size
		Expr *size_expr {};
		u32 size {};

		ACCEPT

		u32 get_size() override;

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;

		llvm::Constant* get_aggregate_constant(EmitCtx &ctx, llvm::ArrayRef<llvm::Constant *> init_list) const override;

		std::string_view get_type_name() const override;

		bool is_equal(Type const *other) const override;

		llvm::Value *op_binary(EmitCtx &ctx, TokenType op, llvm::Value *lhs, llvm::Value *rhs) const override;

		llvm::Value *access_member(EmitCtx &ctx, llvm::Value *ptr, llvm::Value *accessor, std::string_view symbol) const override;
		std::optional<MemberInfo> get_member(std::string_view symbol) const override;

		bool can_coerce_into(Type const *desired_type) const override;
		llvm::Value *op_coerce(EmitCtx &ctx, llvm::Value *value, Type *desired_type) const override;

		llvm::Value *op_slice(EmitCtx &ctx, llvm::Value *array, llvm::Value *target, llvm::Value *start, llvm::Value *end) const override;

		int get_member_index(std::string_view member) const;
	};

	struct Slice : BaseArray
	{
		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;

		std::string_view get_type_name() const override;

		bool is_equal(Type const *other) const override;

		llvm::Value *op_index(EmitCtx &ctx, llvm::Value *target, llvm::Value *index) const override;

		llvm::Value *access_member(EmitCtx &ctx, llvm::Value *ptr, llvm::Value *accessor, std::string_view symbol) const override;
	};

	struct StructField
	{
		Symbol symbol;
		TypeMeta type_meta;
		Type *type;
	};

	struct Struct : Type
	{
		Symbol symbol;
		std::vector<StructField> fields;

		u32 get_size() override;

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;

		llvm::Constant* get_aggregate_constant(EmitCtx &ctx, llvm::ArrayRef<llvm::Constant *> init_list) const override;

		std::string_view get_type_name() const override;

		llvm::Value *get_default_value(llvm::LLVMContext *ctx) const override;

		bool is_struct() const override
		{
			return true;
		}

		bool is_equal(Type const *other) const override;

		llvm::Value *access_member(EmitCtx &ctx, llvm::Value *ptr, llvm::Value *accessor, std::string_view symbol) const override;
		std::optional<MemberInfo> get_member(std::string_view symbol) const override;

		ACCEPT
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
	static const Pointer VoidPointerType {const_cast<Void*>(&VoidType)};

	struct Str : Pointer
	{
		DEFAULT_TYPE_EQUAL(Str)

		Str() :
			Pointer{const_cast<Char*>(&CharType)}
		{}

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;

		std::string_view get_type_name() const override
		{
			return "str";
		}

		llvm::Value *get_default_value(llvm::LLVMContext *ctx) const override;

		bool can_coerce_into(Type const *desired_type) const override
		{
			return false;
		}
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

	enum class FnFlags : u8
	{
		Extern = 1 << 0,
		Inline = 1 << 1,
		Private = 1 << 2,
		ArrowFn = 1 << 3,
	};

	PARS_FLAGIFY(FnFlags);

	struct FnSignature
	{
		std::vector<VarDeclStmt*> parameters;
		// the amount of non default parameters
		u32 callable_arity {};
		bool is_variadic {};
		Type *return_type {};
		TypeMeta return_type_meta;

		llvm::Function* emit(EmitCtx &ctx, std::string_view name, FnFlags flags) const;
	};

	struct FnType : Type
	{
		Symbol symbol;
		FnSignature signature;
		BlockStmt *body;
		FnFlags flags {};

		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;

		bool is_callable() const override
		{
			return true;
		}

		std::string_view get_type_name() const override
		{
			return symbol.name;
		}

		llvm::Value *get_default_value(llvm::LLVMContext *ctx) const override;

		llvm::FunctionType* get_fn_llvm_type(llvm::LLVMContext *ctx) const;

		std::optional<CallInfo> get_call_info() override
		{
			return CallInfo
			{
				signature.parameters,
				signature.return_type_meta,
				signature.is_variadic,
				signature.callable_arity
			};
		}

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;

		llvm::Value *op_call(EmitCtx &ctx, llvm::Value *callable, llvm::ArrayRef<llvm::Value *> args) const override;

		bool is_equal(Type const *other) const override;

		ACCEPT
	};

	template<class T>
	T* produce_type(Type *type)
	{
		if (auto *alias = dynamic_cast<AliasType*>(type))
		{
			return (T*)alias->type;
		}

		return (T*)type;
	}
}

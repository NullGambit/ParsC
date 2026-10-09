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
	struct EnumLiteralExpr;
	struct FnCollection;
	constexpr auto IS_SIGNED = true;

	enum class MemberAccess
	{
		Public,
		Private,
		Readonly,
	};

	// represents a member of an object. can be a field or a method
	struct MemberInfo
	{
		std::string_view name;
		Type *type;
		MemberAccess access;
		MutSet self_mut_set;
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
		enum class Kind
		{
			Unresolved,
			Void,
			UserDef,
			Integral,
			Alias,
			Integer,
			Float,
			Bool,
			Char,
			Pointer,
			Packed,
			Struct,
			Array,
			Slice,
			Str,
			Fn,
			Enum,
			Range,
		} kind;

		Type(Kind kind) :
			Node{BaseKind::Type},
			kind{kind}
		{}

		virtual bool is_equal(Type const *other) const = 0;
		virtual bool is_primitive() const { return false; }

		virtual llvm::Type* get_llvm_type(llvm::LLVMContext *ctx) const = 0;

		virtual llvm::Constant* get_aggregate_constant(EmitCtx &ctx, llvm::ArrayRef<llvm::Constant *> init_list) const
		{
			return nullptr;
		}

		virtual llvm::Constant* get_constant_from_literal(EmitCtx &ctx, u64 n) const
		{
			return nullptr;
		}

		virtual llvm::Constant* get_constant_from_literal(EmitCtx &ctx, i64 n) const
		{
			return nullptr;
		}

		virtual llvm::Constant* get_constant_from_literal(EmitCtx &ctx, f64 n) const
		{
			return nullptr;
		}

		// will return the real type that this type node holds. in case of a wrapper such as alias it will be its
		// aliased type but in other cases it will be the type itself.
		virtual Type* get_real_type() const { return const_cast<Type*>(this); }

		virtual std::string_view get_type_name() const = 0;
		// currently useless but should be reused for comp_eval
		virtual u32 get_size() { return 1; }
		virtual llvm::Value* get_default_value(llvm::LLVMContext *ctx) const = 0;
		virtual llvm::Value* get_property(llvm::LLVMContext *ctx, std::string_view name);
		virtual Type* get_inner() const { return nullptr; }
		virtual bool is_ptr() const { return false; }
		virtual bool is_int() const { return false; }
		virtual bool is_array() const { return false; }
		virtual bool is_struct() const { return false; }
		virtual bool is_callable() const { return false; }

		virtual llvm::Value* access_member(EmitCtx &ctx, llvm::Value *ptr, llvm::Value *accessor, std::string_view symbol) const { return nullptr; }
		virtual std::optional<MemberInfo> get_member(std::string_view symbol) const { return {}; }
		virtual std::optional<MemberInfo> get_method(std::string_view symbol) const { return {}; }
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

#define TYPE_AS(TV, T) ((TV) != nullptr && (TV)->kind == Type::Kind::T ? static_cast<T##Type*>((TV)) : nullptr)

	Type* produce_type(Type const *type);

	template<class T>
	T* produce_type(Type const *type)
	{
		return dynamic_cast<T*>(produce_type(type));
	}

	template<class T>
	T* produce_type(Type *type)
	{
		return dynamic_cast<T*>(produce_type(type));
	}

	template<class T>
	bool is_type_same(Type const *type)
	{
		auto *real = produce_type<T>(type);

		return real != nullptr;
	}

	bool check_type_equality(Type const *a_type, Type  const *b_type);

	bool is_assignable_from(Type const *from, Type  const *to);

	template<class T>
	T const * types_match(Type const *a_type, Type  const *b_type)
	{
		auto *a = produce_type<const T>(a_type);
		auto *b = produce_type<const T>(b_type);

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
	auto other_int = produce_type<T>(other);								\
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
	struct UnresolvedSymbolType : Type
	{
		std::string_view symbol;

		UnresolvedSymbolType() :
			Type{Kind::Unresolved}
		{}

		bool is_equal(Type const *other) const override { return false; }

		llvm::Type * get_llvm_type(llvm::LLVMContext *ctx) const override { return nullptr; }

		std::string_view get_type_name() const override { return symbol; }

		llvm::Value * get_default_value(llvm::LLVMContext *ctx) const override { return nullptr; }

		ACCEPT
	};

	struct VoidType : Type
	{
		DEFAULT_TYPE_EQUAL(VoidType)

		VoidType() :
			Type{Kind::Void}
		{}

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

	struct UserDefType : Type
	{
		ImplStmt *impl {};

		UserDefType(Kind kind) :
			Type{kind}
		{}

		std::optional<MemberInfo> get_method(std::string_view symbol) const;
	};

	struct IntegralType : Type
	{
		u8 bits;
		bool is_signed;
		std::string_view type_name;

		IntegralType(Kind kind, u8 bits, bool is_signed, std::string_view type_name) :
			Type{kind},
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

		DEFAULT_INTEGRAL_EQUAL(IntegralType)
	};

	struct AliasType : UserDefType
	{
		Symbol symbol;
		TypeMeta type;
		bool is_distinct = false;

		AliasType() :
			UserDefType{Kind::Alias}
		{}

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;
		std::string_view get_type_name() const override;
		bool is_equal(Type const *other) const override;

		Type *get_real_type() const override;

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
		bool is_int() const override;

		std::optional<MemberInfo> get_member(std::string_view symbol) const override;
		std::optional<MemberInfo> get_method(std::string_view symbol) const override;
		llvm::Value* access_member(EmitCtx &ctx, llvm::Value *ptr, llvm::Value *accessor, std::string_view symbol) const override;

		llvm::Value* emit(EmitCtx &ctx, EmitParams params) override;

		bool is_primitive() const override;

		llvm::Constant*get_aggregate_constant(EmitCtx &ctx, llvm::ArrayRef<llvm::Constant*> init_list) const override;

		llvm::Value* get_property(llvm::LLVMContext *ctx, std::string_view name) override;

		llvm::Value* op_slice(EmitCtx &ctx, llvm::Value *array, llvm::Value *target, llvm::Value *start,
			llvm::Value *end) const override;

		bool can_coerce_into(Type const *desired_type) const override;

		bool is_iterable() const override;

		std::span<Type*> get_iter_bindings() const override;

		llvm::Value* iter_emit_init(EmitCtx &ctx, Expr *iterable, std::span<llvm::Value*> vars) const override;

		llvm::Value* iter_emit_update(EmitCtx &ctx, Expr *iterable, std::span<llvm::Value*> vars) const override;

		llvm::Value* iter_emit_condition(EmitCtx &ctx, Expr *iterable, std::span<llvm::Value*> vars) const override;

		llvm::Constant * get_constant_from_literal(EmitCtx &ctx, u64 n) const override;

		llvm::Constant * get_constant_from_literal(EmitCtx &ctx, i64 n) const override;

		llvm::Constant * get_constant_from_literal(EmitCtx &ctx, f64 n) const override;

		ACCEPT
	};

	struct IntegerType : IntegralType
	{
		IntegerType(u8 bits, bool is_signed, std::string_view type_name) :
			IntegralType{Kind::Integer, bits, is_signed, type_name}
		{}

		IntegerType(Kind kind, u8 bits, bool is_signed, std::string_view type_name) :
			IntegralType{kind, bits, is_signed, type_name}
		{}

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;

		llvm::Value *op_binary(EmitCtx &ctx, TokenType op, llvm::Value *lhs, llvm::Value *rhs) const override;
		llvm::Value *op_unary(EmitCtx &ctx, TokenType op, llvm::Value *rhs) const override;

		llvm::Constant *get_constant_from_literal(EmitCtx &ctx, i64 n) const override;
		llvm::Constant *get_constant_from_literal(EmitCtx &ctx, u64 n) const override;
		llvm::Constant *get_constant_from_literal(EmitCtx &ctx, f64 n) const override;

		llvm::Value *op_abs(EmitCtx &ctx, llvm::Value *value) const override;

		bool is_int() const override
		{
			return true;
		}

		DEFAULT_INTEGRAL_EQUAL(IntegerType)
	};

	struct FloatType : IntegralType
	{
		FloatType(u8 bits, bool is_signed, std::string_view type_name) :
			IntegralType{Kind::Float, bits, is_signed, type_name}
		{}

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;

		llvm::Value *op_binary(EmitCtx &ctx, TokenType op, llvm::Value *lhs, llvm::Value *rhs) const override;
		llvm::Value *op_unary(EmitCtx &ctx, TokenType op, llvm::Value *rhs) const override;
		llvm::Value *get_default_value(llvm::LLVMContext *ctx) const override;

		llvm::Value *op_abs(EmitCtx &ctx, llvm::Value *value) const override;

		llvm::Constant *get_constant_from_literal(EmitCtx &ctx, f64 n) const override;
		llvm::Constant *get_constant_from_literal(EmitCtx &ctx, i64 n) const override;

		DEFAULT_INTEGRAL_EQUAL(FloatType)
	};

	struct BoolType : IntegralType
	{
		BoolType(u8 bits, bool is_signed, std::string_view type_name) :
			IntegralType{Kind::Bool, bits, is_signed, type_name}
		{}

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;

		llvm::Value *op_binary(EmitCtx &ctx, TokenType op, llvm::Value *lhs, llvm::Value *rhs) const override;
		llvm::Value *op_unary(EmitCtx &ctx, TokenType op, llvm::Value *rhs) const override;

		DEFAULT_INTEGRAL_EQUAL(BoolType)
	};

	struct CharType : IntegralType
	{
		CharType(u8 bits, bool is_signed, std::string_view type_name) :
			IntegralType{Kind::Char, bits, is_signed, type_name}
		{}

		llvm::Value *op_binary(EmitCtx &ctx, TokenType op, llvm::Value *lhs, llvm::Value *rhs) const override;

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;

		DEFAULT_INTEGRAL_EQUAL(CharType)
	};

	struct PointerType : IntegerType
	{
		ACCEPT

		PointerType() :
			IntegerType{Kind::Pointer, 64, IS_SIGNED, "pointer"}
		{}

		explicit PointerType(Type *inner) :
			IntegerType{64, IS_SIGNED, "pointer"},
			m_inner{inner}
		{}

		explicit PointerType(Kind kind, Type *inner) :
			IntegerType{kind, 64, IS_SIGNED, "pointer"},
			m_inner{inner}
		{}

		void set_inner(Type *type, bool no_name = false);

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;

		std::string_view get_type_name() const override;

		std::optional<MemberInfo> get_member(std::string_view symbol) const override;
		std::optional<MemberInfo> get_method(std::string_view symbol) const override;
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
			return m_inner;
		}

		Type*& get_inner_ref()
		{
			return m_inner;
		}

		bool is_equal(Type const *other) const override;

		bool is_ptr() const override
		{
			return true;
		}

		bool is_int() const override
		{
			return false;
		}

	private:
		Type *m_inner {};
		std::string_view m_name;
	};

	struct PackedType : Type
	{
		PackedType() :
			Type{Kind::Packed}
		{}

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

	static PackedType PackedType {};

	struct BaseArrayType : Type
	{
		ACCEPT

		BaseArrayType(Kind kind) :
			Type{kind}
		{}

		llvm::Value *get_default_value(llvm::LLVMContext *ctx) const override;

		llvm::Value *op_index(EmitCtx &ctx, llvm::Value *target, llvm::Value *index) const override;

		std::optional<MemberInfo> get_member(std::string_view symbol) const override;

		void set_inner(Type *type, bool no_name = false);

		Type *get_inner() const override;
		Type*& get_inner_ref();

		bool is_array() const override
		{
			return true;
		}

		llvm::Value *do_op_index(EmitCtx &ctx, llvm::Value *target, llvm::Value *index, llvm::Type *type) const;

		bool is_iterable() const override
		{
			return true;
		}

		std::string_view get_type_name() const override;

		std::span<Type*> get_iter_bindings() const override;
		llvm::Value *iter_emit_init(EmitCtx &ctx, Expr *iterable, std::span<llvm::Value *> vars) const override;
		llvm::Value *iter_emit_condition(EmitCtx &ctx, Expr *iterable, std::span<llvm::Value *> vars) const override;
		llvm::Value *iter_emit_update(EmitCtx &ctx, Expr *iterable, std::span<llvm::Value *> vars) const override;

	protected:
		Type *m_element_type {};
		std::string_view m_name;

		// will be called when writing to the name buffer to generate [<this>]T
		virtual void write_to_name_buffer() {};
	};

	constexpr auto UNSIZED_ARRAY = UINT32_MAX;

	struct ArrayType : BaseArrayType
	{
		std::vector<std::string_view> members;
		// a temporary expression that must compile to a constant and will be assigned to size
		Expr *size_expr {};
		u32 size {};

		ArrayType() :
			BaseArrayType{Kind::Array}
		{}

		ACCEPT

		u32 get_size() override;

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;

		llvm::Constant* get_aggregate_constant(EmitCtx &ctx, llvm::ArrayRef<llvm::Constant *> init_list) const override;

		bool is_equal(Type const *other) const override;

		llvm::Value *op_binary(EmitCtx &ctx, TokenType op, llvm::Value *lhs, llvm::Value *rhs) const override;

		llvm::Value *access_member(EmitCtx &ctx, llvm::Value *ptr, llvm::Value *accessor, std::string_view symbol) const override;
		std::optional<MemberInfo> get_member(std::string_view symbol) const override;

		bool can_coerce_into(Type const *desired_type) const override;
		llvm::Value *op_coerce(EmitCtx &ctx, llvm::Value *value, Type *desired_type) const override;

		llvm::Value *op_slice(EmitCtx &ctx, llvm::Value *array, llvm::Value *target, llvm::Value *start, llvm::Value *end) const override;

		int get_member_index(std::string_view member) const;

	protected:
		void write_to_name_buffer() override;
	};

	struct SliceType : BaseArrayType
	{
		SliceType() :
			BaseArrayType{Kind::Slice}
		{}

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;

		bool is_equal(Type const *other) const override;

		llvm::Value *op_index(EmitCtx &ctx, llvm::Value *target, llvm::Value *index) const override;

		llvm::Value *access_member(EmitCtx &ctx, llvm::Value *ptr, llvm::Value *accessor, std::string_view symbol) const override;
	};

	struct StructFieldInfo
	{
		Symbol symbol;
		TypeMeta type;
	};

	enum class StructFlags
	{
		Resolved = 1 << 0,
	};

	PARS_FLAGIFY(StructFlags);

	struct StructType : UserDefType
	{
		Symbol symbol;
		std::vector<StructFieldInfo> fields;
		llvm::StructType *llvm_type {};
		StructFlags flags;

		StructType() :
			UserDefType{Kind::Struct}
		{}

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

	static const VoidType VOID_TYPE {};
	static const IntegerType I8_TYPE {8, IS_SIGNED, "i8"};
	static const IntegerType U8_TYPE {8, !IS_SIGNED, "u8"};
	static const IntegerType I16_TYPE {16, IS_SIGNED, "i16"};
	static const IntegerType U16_TYPE {16, !IS_SIGNED, "u16"};
	static const IntegerType I32_TYPE {32, IS_SIGNED, "i32"};
	static const IntegerType U32_TYPE {32, !IS_SIGNED, "u32"};
	static const IntegerType I64_TYPE {64, IS_SIGNED, "i64"};
	static const IntegerType U64_TYPE {64, !IS_SIGNED, "u64"};
	static const FloatType F32_TYPE {32, IS_SIGNED, "f32"};
	static const FloatType F64_TYPE {64, IS_SIGNED, "f64"};
	static const BoolType BOOL_TYPE {1, !IS_SIGNED, "bool"};
	static const CharType CHAR_TYPE {8, IS_SIGNED, "char"};
	static const CharType UCHAR_TYPE {8, !IS_SIGNED, "uchar"};
	static const PointerType VOID_POINTER_TYPE {const_cast<VoidType*>(&VOID_TYPE)};

	struct StrType : PointerType
	{
		DEFAULT_TYPE_EQUAL(StrType)

		StrType() :
			PointerType{Kind::Str, const_cast<CharType*>(&CHAR_TYPE)}
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

	static const StrType StrType {};

	struct RangeType : Type
	{
		DEFAULT_TYPE_EQUAL(RangeType)

		RangeType() :
			Type{Kind::Range}
		{}

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
		Static = 1 << 4,
		Resolved = 1 << 5,
	};

	PARS_FLAGIFY(FnFlags);

	using FnParams = std::vector<VarDeclStmt*>;
	struct FnType;

	struct FnSignature
	{
		FnParams parameters;
		// the amount of non default parameters
		u32 callable_arity {};
		bool is_variadic {};
		TypeMeta return_type {};

		llvm::Function* emit(EmitCtx &ctx, std::string_view name, FnFlags flags) const;
	};

	struct FnType : Type
	{
		Symbol symbol;
		std::string mangled_name;
		FnSignature signature;
		BlockStmt *body;
		FnFlags flags {};
		FnCollection *collection;

		FnType() :
			Type{Kind::Fn}
		{}

		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;

		std::string_view get_fn_name() const;

		bool is_callable() const override
		{
			return true;
		}

		std::string_view get_type_name() const override
		{
			return symbol.name;
		}

		llvm::Function* get_llvm_fn(EmitCtx &ctx) const;

		llvm::Value *get_default_value(llvm::LLVMContext *ctx) const override;

		llvm::FunctionType* get_fn_llvm_type(llvm::LLVMContext *ctx) const;

		std::optional<CallInfo> get_call_info() override
		{
			return CallInfo
			{
				signature.parameters,
				signature.return_type,
				signature.is_variadic,
				signature.callable_arity
			};
		}

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;

		llvm::Value *op_call(EmitCtx &ctx, llvm::Value *callable, llvm::ArrayRef<llvm::Value *> args) const override;

		bool is_equal(Type const *other) const override;

		ACCEPT
	};

	struct EnumVariant
	{
		Symbol symbol;
		u64 value;
	};

	struct EnumType : UserDefType
	{
		Symbol symbol;
		u64 default_value = UINT64_MAX;
		HashMap<std::string_view, EnumVariant> variants;

		EnumType() :
			UserDefType{Kind::Enum}
		{}

		std::string_view get_type_name() const override;

		llvm::Value *get_default_value(llvm::LLVMContext *ctx) const override;

		llvm::Type *get_llvm_type(llvm::LLVMContext *ctx) const override;

		bool is_equal(Type const *other) const override;

		EnumLiteralExpr* get_literal(std::string_view name) const;
		std::optional<u32> get_value(std::string_view name) const;

		llvm::Value *op_cast(EmitCtx &ctx, llvm::Value *value, Type *desired_type) const override;
		llvm::Value *op_binary(EmitCtx &ctx, TokenType op, llvm::Value *lhs, llvm::Value *rhs) const override;
	};
}

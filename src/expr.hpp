#pragma once
#include <bitset>
#include <string_view>
#include <variant>
#include <vector>

#include "emit_context.hpp"
#include "node.hpp"
#include "symbol.hpp"
#include "type_meta.hpp"
#include "visitor.hpp"

#include "llvm/IR/IRBuilder.h"
#include "util/macros.hpp"

namespace pars
{
	struct Module;
	struct FnType;
	struct Type;

	enum class ExprFlags : u8
	{
		Immutable = 1 << 0,
		AlwaysPtr = 1 << 1,
		// a flag for the expression to decide internally if its been resolved or not.
		ResolvedInternal = 1 << 2,
		AlwaysLoad = 1 << 3,
	};

	PARS_FLAGIFY(ExprFlags);

	struct Expr : Node
	{
		ExprFlags flags;
		// represents a positional mutability set. directly mirrors mutability on the type tree this belongs to
		std::bitset<32> mut_set;
		Type *type;

		// The purpose of this enum is to be able to identify any expressions type without reliance on RTTI by using dynamic_cast
		enum class Kind : u8
		{
			Literal,
			Binary,
			Unary,
			Symbol,
			Call,
			Group,
			Type,
			Sizeof,
			Nameof,
			MemberAccess,
			TypeProp,
			Cast,
			Named,
			Abs,
			PtrOp,
			Aggregate,
			AnonInit,
			ArrayLiteral,
			IndexOp,
			StructLiteral,
			SliceLiteral,
			EnumLiteral,
			Slice,
			Keyword,
			Packed,
		} kind;

		Expr(Kind kind) :
			Node{BaseKind::Expr},
			kind{kind}
		{}

		virtual llvm::Value *emit_ptr(EmitCtx &ctx, EmitParams params = {}) { return nullptr; }
		virtual llvm::Constant* emit_constant(EmitCtx &ctx, EmitParams params = {});

		virtual std::string_view get_symbol() { return {}; }

		virtual bool is_ctx_sensitive() { return false; }
		virtual bool is_resolved() { return type != nullptr; }
	};

#define EXPR_AS(E, T) ((E) != nullptr && (E)->kind == Expr::Kind::T ? static_cast<T##Expr*>((E)) : nullptr)

	using LiteralExprValue = std::variant
	<
		i32,
		f32,
		std::string_view,
		bool,
		char,
		std::nullptr_t
	>;

	struct LiteralExpr : Expr
	{
		LiteralExprValue value;

		LiteralExpr() :
			Expr{Kind::Literal}
		{}

		explicit LiteralExpr(LiteralExprValue value) :
			Expr{Kind::Literal},
			value{value}
		{}

		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;

		std::optional<i64> get_int() const;

		bool is_ctx_sensitive() override
		{
			return true;
		}

		bool is_resolved() override
		{
			return has_flag(flags, ExprFlags::ResolvedInternal);
		}

		std::string_view get_symbol() override
		{
			return token.lexeme;
		}

		ACCEPT
	};

	struct BinaryExpr : Expr
	{
		Expr *left;
		TokenType op;
		Expr *right;

		BinaryExpr() :
			Expr{Kind::Binary}
		{}

		BinaryExpr(Expr *left, TokenType op, Expr *right) :
			Expr{Kind::Binary},
			left{left},
			op{op},
			right{right}
		{}

		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;

		bool is_ctx_sensitive() override
		{
			return left->is_ctx_sensitive() || right->is_ctx_sensitive();
		}

		bool is_resolved() override
		{
			return left->is_resolved() && right->is_resolved();
		}

		ACCEPT
	};

	struct UnaryExpr : Expr
	{
		char op;
		Expr *right;

		UnaryExpr() :
			Expr{Kind::Unary}
		{}

		UnaryExpr(char op, Expr *right) :
			Expr{Kind::Unary},
			op{op},
			right{right}
		{}

		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;

		ACCEPT
	};

	// for not only variables but also functions to be used when taking the address of a function
	struct SymbolExpr : Expr
	{
		Node *symbol_node;
		std::string_view symbol;

		SymbolExpr() :
			Expr{Kind::Symbol}
		{}

		SymbolExpr(std::string_view symbol) :
			Expr{Kind::Symbol},
			symbol{symbol}
		{}

		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;

		llvm::Value *emit_ptr(EmitCtx &ctx, EmitParams params = {}) override;

		std::string_view get_symbol() override;

		bool is_ctx_sensitive() override
		{
			return true;
		}

		ACCEPT
	};

	struct CallExpr : Expr
	{
		Expr *callable;
		std::vector<Expr*> arguments;

		CallExpr() :
			Expr{Kind::Call}
		{}

		CallExpr(Expr *callable, std::vector<Expr*> &&arguments) :
			Expr{Kind::Call},
			callable{callable},
			arguments{arguments}
		{}

		llvm::Value *emit_ptr(EmitCtx &ctx, EmitParams params = {}) override;
		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;

		std::string_view get_symbol() override
		{
			return callable->get_symbol();
		}

		ACCEPT
	};

	struct GroupExpr : Expr
	{
		Expr* inner;

		GroupExpr() :
			Expr{Kind::Group}
		{}

		GroupExpr(Expr *inner) :
			Expr{Kind::Group},
			inner{inner}
		{}

		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;

		llvm::Value *emit_ptr(EmitCtx &ctx, EmitParams params = {}) override;

		std::string_view get_symbol() override
		{
			return inner->get_symbol();
		}

		ACCEPT
	};

	struct TypeExpr : Expr
	{
		ACCEPT

		TypeExpr() :
			Expr{Kind::Type}
		{}
	};

	struct SizeofExpr : Expr
	{
		Expr *expr;

		SizeofExpr() :
			Expr{Kind::Sizeof}
		{}

		SizeofExpr(Expr *expr) :
			Expr{Kind::Sizeof},
			expr{expr}
		{}

		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;

		ACCEPT
	};

	struct NameofExpr : Expr
	{
		Expr *expr;

		NameofExpr() :
			Expr{Kind::Nameof}
		{}

		NameofExpr(Expr *expr) :
			Expr{Kind::Nameof},
			expr{expr}
		{}

		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;

		ACCEPT
	};

	// mainly used for attributes
	// empty because all nodes store a token already
	struct KeywordExpr : Expr
	{
		KeywordExpr() :
			Expr{Kind::Keyword}
		{}
	};

	struct MemberAccessExpr : Expr
	{
		Expr *target;
		Expr *accessor;
		bool is_static_access {};
		bool is_method {};

		MemberAccessExpr() :
			Expr{Kind::MemberAccess}
		{}

		llvm::Value* emit(EmitCtx& ctx, EmitParams params = {}) override;
		llvm::Value *emit_ptr(EmitCtx &ctx, EmitParams params = {}) override;

		std::string_view get_symbol() override;

		ACCEPT
	};

	struct TypePropExpr : Expr
	{
		std::string_view property_name;

		TypePropExpr() :
			Expr{Kind::TypeProp}
		{}

		llvm::Value* emit(EmitCtx& ctx, EmitParams params = {}) override;
	};

	struct CastExpr : Expr
	{
		TypeMeta cast_type;
		Expr *target;
		// useful for correct integer casting when doing code gen
		Type *original_type;

		CastExpr() :
			Expr{Kind::Cast}
		{}

		llvm::Value* emit(EmitCtx& ctx, EmitParams params = {}) override;

		ACCEPT
	};

	struct NamedExpr : Expr
	{
		std::string_view name;
		Expr *value;

		NamedExpr() :
			Expr{Kind::Named}
		{}

		ACCEPT

		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;
	};

	struct InitializerElement
	{
		Expr *expr {};
		u32 index = UINT32_MAX;
	};

	struct AbsExpr : Expr
	{
		Expr *value;

		AbsExpr() :
			Expr{Kind::Abs}
		{}

		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;

		ACCEPT
	};

	struct PtrOpExpr : Expr
	{
		TokenType op;
		Expr *target;

		PtrOpExpr() :
			Expr{Kind::PtrOp}
		{}

		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;

		llvm::Value *emit_ptr(EmitCtx &ctx, EmitParams params = {}) override;

		ACCEPT
	};

	struct PackedExpr : Expr
	{
		PackedExpr() :
			Expr{Kind::Packed}
		{}

		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;

		ACCEPT
	};

	// represents a list of expressions for initializers such as structs, arrays, tuples.
	// is in a format that is useful for reordering when using named initializers.
	// the second of the pair will later on be set to its correct order to be used in code gen.
	using InitializerList = std::vector<InitializerElement>;

	struct AggregateExpr : Expr
	{
		InitializerList initializers;

		AggregateExpr() :
			Expr{Kind::Aggregate}
		{}

		AggregateExpr(Kind kind) :
			Expr{kind}
		{}

		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;
		llvm::Constant *emit_constant(EmitCtx &ctx, EmitParams params) override;

		ACCEPT
	};

	// represents any brace initialized value. also used for default value initializations
	struct AnonInitExpr : AggregateExpr
	{
		AnonInitExpr() :
			AggregateExpr{Kind::AnonInit}
		{}

		bool is_ctx_sensitive() override
		{
			return true;
		}

		ACCEPT
	};

	struct ArrayLiteralExpr : AggregateExpr
	{
		Expr *type_specifier {};

		ArrayLiteralExpr() :
			AggregateExpr{Kind::ArrayLiteral}
		{}

		bool is_ctx_sensitive() override
		{
			return type_specifier == nullptr && initializers.empty();
		}

		ACCEPT
	};

	struct IndexOpExpr : Expr
	{
		Expr *lhs;
		Expr *index;

		IndexOpExpr() :
			Expr{Kind::IndexOp}
		{}

		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;

		llvm::Value *emit_ptr(EmitCtx &ctx, EmitParams params = {}) override;

		std::string_view get_symbol() override
		{
			return lhs->get_symbol();
		}

		ACCEPT
	};

	// TODO perhaps generalize this so any type such as an i32 can be initialized with braces
	struct StructLiteral : AggregateExpr
	{
		std::string_view name;

		StructLiteral() :
			AggregateExpr{Kind::StructLiteral}
		{}

		ACCEPT
	};

	struct SliceExpr : Expr
	{
		Expr *lhs {};
		Expr *start {};
		Expr *end {};

		SliceExpr() :
			Expr{Kind::Slice}
		{}

		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;
		llvm::Value *emit_ptr(EmitCtx &ctx, EmitParams params = {}) override;

		ACCEPT

	private:
		llvm::Value *m_cached_result {};
	};

	struct EnumLiteralExpr : Expr
	{
		u32 value {};

		EnumLiteralExpr() :
			Expr{Kind::EnumLiteral}
		{}

		llvm::Constant *emit_constant(EmitCtx &ctx, EmitParams params) override;
		llvm::Value *emit(EmitCtx &ctx, EmitParams params) override;
	};
}

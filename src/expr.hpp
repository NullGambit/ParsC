#pragma once
#include <string_view>
#include <variant>
#include <vector>

#include "emit_context.hpp"
#include "node.hpp"
#include "symbol.hpp"
#include "visitor.hpp"

#include "llvm/IR/IRBuilder.h"

namespace pars
{
	struct Module;
	struct FnDecl;
	struct Type;

	enum class ExprMutability
	{
		Mutable,
		Immutable,
	};

	struct Expr : Node
	{
		Type *type;

		virtual llvm::Value *emit_ptr(EmitCtx &ctx, EmitParams params = {}) { return nullptr; }

		virtual std::string_view get_symbol() { return {}; }
	};

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

		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;

		ACCEPT
	};

	struct BinaryExpr : Expr
	{
		Expr *left;
		TokenType op;
		Expr *right;

		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;

		ACCEPT
	};

	struct UnaryExpr : Expr
	{
		char op;
		Expr *right;

		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;

		ACCEPT
	};

	// for not only variables but also functions to be used when taking the address of a function
	struct SymbolExpr : Expr
	{
		Node *symbol_node;
		std::string_view symbol;

		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;

		llvm::Value *emit_ptr(EmitCtx &ctx, EmitParams params = {}) override;

		std::string_view get_symbol() override
		{
			return symbol;
		}

		ACCEPT
	};

	struct CallExpr : Expr
	{
		FnDecl *prototype;
		std::string_view symbol;
		std::vector<Expr*> arguments;

		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;

		std::string_view get_symbol() override
		{
			return symbol;
		}

		ACCEPT
	};

	struct GroupExpr : Expr
	{
		Expr* inner;

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
	};

	struct SizeofExpr : Expr
	{
		Expr *expr;

		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;

		ACCEPT
	};

	// mainly used for attributes
	// empty because all nodes store a token already
	struct KeywordExpr : Expr
	{
	};

	struct MemberAccessExpr : Expr
	{
		Expr *target;
		Expr *accessor;

		llvm::Value* emit(EmitCtx& ctx, EmitParams params = {}) override;
		llvm::Value *emit_ptr(EmitCtx &ctx, EmitParams params = {}) override;

		std::string_view get_symbol() override;

		ACCEPT
	};

	struct TypePropExpr : Expr
	{
		std::string_view property_name;

		llvm::Value* emit(EmitCtx& ctx, EmitParams params = {}) override;
	};

	struct CastExpr : Expr
	{
		Expr *type_expr;
		Expr *target;
		// useful for correct integer casting when doing code gen
		Type *original_type;

		llvm::Value* emit(EmitCtx& ctx, EmitParams params = {}) override;

		ACCEPT
	};

	struct NamedExpr : Expr
	{
		std::string_view name;
		Expr *value;

		ACCEPT

		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;
	};

	using BraceInitList = std::vector<std::pair<Expr*, u32>>;

	// represents any brace initialized value. also used for default value initializations
	struct AnonInitExpr : Expr
	{
		BraceInitList values;

		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;

		ACCEPT
	};

	struct AbsExpr : Expr
	{
		Expr *value;

		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;

		ACCEPT
	};

	struct PtrOpExpr : Expr
	{
		TokenType op;
		Expr *target;

		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;

		llvm::Value *emit_ptr(EmitCtx &ctx, EmitParams params = {}) override;

		ACCEPT
	};

	struct PackedExpr : Expr
	{
		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;

		ACCEPT
	};

	struct ArrayLiteralExpr : Expr
	{
		std::vector<Expr*> elements;

		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;

		ACCEPT
	};

	struct IndexOpExpr : Expr
	{
		Expr *lhs;
		Expr *index;

		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;

		llvm::Value *emit_ptr(EmitCtx &ctx, EmitParams params = {}) override;

		ACCEPT
	};

	struct StructLiteral : Expr
	{
		std::string_view name;
		BraceInitList initializers;

		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;

		ACCEPT
	};

	struct SliceExpr : Expr
	{
		Expr *lhs {};
		Expr *start {};
		Expr *end {};

		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;
		llvm::Value *emit_ptr(EmitCtx &ctx, EmitParams params = {}) override;

		ACCEPT

	private:
		llvm::Value *m_cached_result {};
	};
}

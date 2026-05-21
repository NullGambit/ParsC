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

	struct Expr : Node
	{
		Type *type;
	};

	using LiteralExprValue = std::variant
	<
		i32,
		f32,
		std::string_view,
		bool,
		char
	>;

	struct LiteralExpr : Expr
	{
		LiteralExprValue value;

		llvm::Value *emit(EmitCtx &ctx) override;

		ACCEPT
	};

	struct BinaryExpr : Expr
	{
		Expr* left;
		char op;
		Expr* right;

		llvm::Value *emit(EmitCtx &ctx) override;

		ACCEPT
	};

	struct UnaryExpr : Expr
	{
		char op;
		Expr* right;

		llvm::Value *emit(EmitCtx &ctx) override;

		ACCEPT
	};

	// for not only variables but also functions to be used when taking the address of a function
	struct SymbolExpr : Expr
	{
		Node *symbol_node;
		std::string_view symbol;

		llvm::Value *emit(EmitCtx &ctx) override;

		ACCEPT
	};

	struct CallExpr : Expr
	{
		FnDecl *prototype;
		std::string_view symbol;
		std::vector<Expr*> arguments;

		llvm::Value *emit(EmitCtx &ctx) override;

		ACCEPT
	};

	struct GroupExpr : Expr
	{
		Expr* expr;

		llvm::Value *emit(EmitCtx &ctx) override;

		ACCEPT
	};

	struct TypeExpr : Expr
	{
		ACCEPT
	};

	struct SizeofExpr : Expr
	{
		Expr *expr;

		llvm::Value *emit(EmitCtx &ctx) override;

		ACCEPT
	};

	// mainly used for attributes
	// empty because all nodes store a token already
	struct KeywordExpr : Expr
	{
	};
}

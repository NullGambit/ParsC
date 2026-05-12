#pragma once
#include <string_view>
#include <vector>

#include "emit_context.hpp"
#include "node.hpp"
#include "symbol.hpp"
#include "type.hpp"
#include "util/macros.hpp"

namespace pars
{
	struct Type;
	struct Expr;

	struct Stmt : Node
	{

	};

	enum class VarFlags : u8
	{
		Const = 1 << 0,
		Static = 1 << 1,
		Volatile = 1 << 2,
	};

	PARSE_FLAGIFY(VarFlags);

	struct VarDeclStmt : Stmt
	{
		Symbol symbol;
		Type *type;
		Expr* initializer;
		VarFlags flags;

		llvm::Value *emit(EmitCtx &ctx) override;

		ACCEPT
	};

	struct FnPrototypeStmt : Stmt
	{
		Symbol symbol;
		std::vector<VarDeclStmt*> parameters;
		Type *return_type;
		bool is_extern = false;

		llvm::Value* emit(EmitCtx &ctx) override;

		ACCEPT
	};

	struct ImportStmt : Stmt
	{
		std::vector<std::string_view> path;
		std::string_view alias;
		std::vector<std::string_view> selective_imports;

		ACCEPT
	};

	struct BlockStmt : Stmt
	{
		FnPrototypeStmt *owner;
		std::vector<Node*> body;

		llvm::Value* emit(EmitCtx &ctx) override;

		ACCEPT
	};

	struct ExprFnStmt : Stmt
	{
		FnPrototypeStmt *owner;
		Expr* expr;

		llvm::Value* emit(EmitCtx &ctx) override;

		ACCEPT
	};

	struct ReturnStmt : Stmt
	{
		Expr* expr {};

		llvm::Value* emit(EmitCtx &ctx) override;

		ACCEPT
	};

	struct PrintlnStmt : Stmt
	{
		Expr* expr {};

		ACCEPT
	};
}

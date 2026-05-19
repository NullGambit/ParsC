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
	struct Module;
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

	PARS_FLAGIFY(VarFlags);

	struct VarDeclStmt : Stmt
	{
		Symbol symbol;
		Type *type;
		Expr* initializer;
		VarFlags flags;

		llvm::Value *emit(EmitCtx &ctx) override;

		ACCEPT
	};

	struct FnSignature
	{
		std::vector<VarDeclStmt*> parameters;
		Type *return_type;

		llvm::Function* emit(EmitCtx &ctx, std::string_view name);
	};

	enum class FnFlags : u8
	{
		Extern = 1 << 0,
		Inline = 1 << 1,
	};

	PARS_FLAGIFY(FnFlags);

	struct FnDecl : Stmt
	{
		Symbol symbol;
		FnSignature signature;
		std::vector<Node*> body;
		FnFlags flags;

		llvm::Value *emit(EmitCtx &ctx) override;
	};

	struct ImportStmt : Stmt
	{
		Module *module;
		std::string_view alias;

		ACCEPT
	};

	struct ReturnStmt : Stmt
	{
		Expr* expr {};

		llvm::Value* emit(EmitCtx &ctx) override;

		ACCEPT
	};
}

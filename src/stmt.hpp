#pragma once
#include <filesystem>
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
		std::string_view type_name;
		Type *type;
		Expr* initializer;
		VarFlags flags;

		llvm::Value *emit(EmitCtx &ctx) override;

		ACCEPT
	};

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
		Type *return_type {};
		std::string_view return_type_name;

		llvm::Function* emit(EmitCtx &ctx, std::string_view name, FnFlags flags);
	};

	struct FnDecl : Stmt
	{
		Symbol symbol;
		FnSignature signature;
		std::vector<Node*> body;
		FnFlags flags;

		llvm::Value *emit(EmitCtx &ctx) override;

		ACCEPT
	};

	struct NamedSymbol
	{
		std::string_view name;
		std::string_view symbol;
	};

	struct ImportStmt : Stmt
	{
		Module *module;
		std::filesystem::path path;
		std::string_view alias;
		std::vector<NamedSymbol> selective_imports;

		ACCEPT
	};

	struct ReturnStmt : Stmt
	{
		Expr* expr {};

		llvm::Value* emit(EmitCtx &ctx) override;

		ACCEPT
	};
}

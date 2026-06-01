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
		// TODO actually implement both static and volatile variables
		Static = 1 << 1,
		Volatile = 1 << 2,
		Global = 1 << 3,
		ShouldAlloca = 1 << 4,
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

	// represents not only assignment but op apply operators (+=, -=, etc)
	// could be an expression but seems like the most natural way to represent assignments is with a statement
	// because unlike C++ assignment cannot be used as an expression
	struct AssignmentStmt : Stmt
	{
		std::string_view symbol;
		VarDeclStmt *lhs;
		Expr *rhs;
		TokenType op;

		llvm::Value* emit(EmitCtx &ctx) override;

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
		// the amount of non default parameters
		u32 callable_arity {};
		Type *return_type {};
		std::string_view return_type_name;

		llvm::Function* emit(EmitCtx &ctx, std::string_view name, FnFlags flags);
	};

	struct BlockStmt : Stmt
	{
		std::vector<Node*> nodes;

		llvm::Value *emit(EmitCtx &ctx) override;

		ACCEPT
	};

	struct FnDecl : Stmt
	{
		Symbol symbol;
		FnSignature signature;
		BlockStmt *body;
		FnFlags flags {};

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

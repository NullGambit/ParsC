#pragma once
#include <filesystem>
#include <string_view>
#include <vector>

#include "emit_context.hpp"
#include "node.hpp"
#include "symbol.hpp"
#include "type.hpp"
#include "type_meta.hpp"
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
		// TODO actually implement static
		Static = 1 << 1,
		Volatile = 1 << 2,
		Global = 1 << 3,
	};

	PARS_FLAGIFY(VarFlags);

	struct VarDeclStmt : Stmt
	{
		Symbol symbol;
		TypeMeta type_meta;
		Type *type;
		Expr* initializer;
		VarFlags flags;

		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;

		llvm::Value *init(EmitCtx &ctx, llvm::Value *value = nullptr);

		bool is_explicitly_typed() const;
		bool is_type_inferred() const;

		ACCEPT
		RECEIVE
	};

	// represents not only assignment but op apply operators (+=, -=, etc)
	// could be an expression but seems like the most natural way to represent assignments is with a statement
	// because unlike C++ assignment cannot be used as an expression
	struct AssignmentStmt : Stmt
	{
		Expr *lhs;
		Expr *rhs;
		TokenType op;

		llvm::Value* emit(EmitCtx &ctx, EmitParams params = {}) override;

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
		bool is_variadic {};
		Type *return_type {};
		TypeMeta return_type_meta;

		llvm::Function* emit(EmitCtx &ctx, std::string_view name, FnFlags flags);
	};

	struct BlockStmt : Stmt
	{
		std::vector<Node*> nodes;

		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;

		ACCEPT
	};

	struct FnDecl : Stmt
	{
		Symbol symbol;
		FnSignature signature;
		BlockStmt *body;
		FnFlags flags {};

		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;

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

	struct TerminatorStmt
	{

	};

	struct ReturnStmt : Stmt, TerminatorStmt
	{
		Expr* expr {};

		llvm::Value* emit(EmitCtx &ctx, EmitParams params = {}) override;

		ACCEPT
	};

	struct BreakStmt : Stmt, TerminatorStmt
	{
		llvm::Value* emit(EmitCtx &ctx, EmitParams params = {}) override;

		ACCEPT
	};

	struct ContinueStmt : Stmt, TerminatorStmt
	{
		llvm::Value* emit(EmitCtx &ctx, EmitParams params = {}) override;

		ACCEPT
	};

	struct IfStmt : Stmt
	{
		Expr* condition;
		BlockStmt *body;
		Node *else_br {};

		llvm::Value* emit(EmitCtx &ctx, EmitParams params = {}) override;

		ACCEPT
	};

	struct CompIfStmt : Stmt
	{
		IfStmt *stmt;

		llvm::Value* emit(EmitCtx &ctx, EmitParams params = {}) override;

		ACCEPT
	};

	struct WhileStmt : Stmt
	{
		Expr* condition;
		BlockStmt *body;

		llvm::Value* emit(EmitCtx &ctx, EmitParams params = {}) override;

		ACCEPT
	};

	struct ForStmt : Stmt
	{
		std::vector<VarDeclStmt*> bindings;
		Expr *iterable;
		BlockStmt *body;

		llvm::Value* emit(EmitCtx &ctx, EmitParams params = {}) override;

		bool has_index() const;

		ACCEPT
	};

}

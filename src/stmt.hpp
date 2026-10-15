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
		enum class Kind
		{
			VarDecl,
			Assignment,
			Block,
			Import,
			Return,
			ConstantReturn,
			Break,
			Continue,
			If,
			CompIf,
			While,
			For,
			Impl,
		} kind;

		Stmt(Kind kind) :
			Node{BaseKind::Stmt},
			kind{kind}
		{}

		virtual bool is_terminator() { return false; }

		static bool classof(const Node *node)
		{
			return node->base_kind == BaseKind::Stmt;
		}
	};

	enum class VarFlags : u32
	{
		// TODO actually implement static
		Static = 1 << 1,
		Volatile = 1 << 2,
		Global = 1 << 3,
		Const = 1 << 4,
		Mutated = 1 << 5,
		Used = 1 << 6,
		Var = 1 << 7,
		// if NoDiagnostics is set this variable should not participate in diagnostics such as warnings
		NoDiagnostics = 1 << 8,
	};

	PARS_FLAGIFY(VarFlags);

	struct VarDeclStmt : Stmt
	{
		Symbol symbol;
		TypeMeta type;
		Expr *initializer {};
		VarFlags flags {};

		CLASSOF(Stmt, VarDecl)

		llvm::Value *emit(EmitCtx &ctx, EmitParams params = {}) override;

		llvm::Value *init(EmitCtx &ctx, llvm::Value *value = nullptr);

		void handle_warnings() const;

		bool is_explicitly_typed() const;
		bool is_type_inferred() const;

		ACCEPT
	};

	// represents not only assignment but op apply operators (+=, -=, etc)
	// could be an expression but seems like the most natural way to represent assignments is with a statement
	// because unlike C++ assignment cannot be used as an expression
	struct AssignmentStmt : Stmt
	{
		Expr *lhs;
		Expr *rhs;
		TokenType op;

		CLASSOF(Stmt, Assignment)

		llvm::Value* emit(EmitCtx &ctx, EmitParams params = {}) override;

		ACCEPT
	};

	struct BlockStmt : Stmt
	{
		std::vector<Node*> nodes;

		CLASSOF(Stmt, Block)

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

		CLASSOF(Stmt, Import)

		ACCEPT
	};

	struct ReturnStmt : Stmt
	{
		Expr* expr {};

		CLASSOF(Stmt, Return)

		ACCEPT

		llvm::Value* emit(EmitCtx &ctx, EmitParams params = {}) override;

		bool is_terminator() override
		{
			return true;
		}
	};

	struct BreakStmt : Stmt
	{
		CLASSOF(Stmt, Break)

		llvm::Value* emit(EmitCtx &ctx, EmitParams params = {}) override;

		ACCEPT

		bool is_terminator() override
		{
			return true;
		}
	};

	struct ContinueStmt : Stmt
	{
		CLASSOF(Stmt, Continue)

		llvm::Value* emit(EmitCtx &ctx, EmitParams params = {}) override;

		ACCEPT

		bool is_terminator() override
		{
			return true;
		}
	};

	struct IfStmt : Stmt
	{
		Expr* condition;
		BlockStmt *body;
		Node *else_br {};

		CLASSOF(Stmt, If)

		llvm::Value* emit(EmitCtx &ctx, EmitParams params = {}) override;

		ACCEPT
	};

	struct CompIfStmt : Stmt
	{
		IfStmt *stmt;

		CLASSOF(Stmt, CompIf)

		llvm::Value* emit(EmitCtx &ctx, EmitParams params = {}) override;

		ACCEPT
	};

	struct WhileStmt : Stmt
	{
		Expr* condition;
		BlockStmt *body;

		CLASSOF(Stmt, While)

		llvm::Value* emit(EmitCtx &ctx, EmitParams params = {}) override;

		ACCEPT
	};

	struct ForStmt : Stmt
	{
		std::vector<VarDeclStmt*> bindings;
		Expr *iterable;
		BlockStmt *body;

		CLASSOF(Stmt, For)

		llvm::Value* emit(EmitCtx &ctx, EmitParams params = {}) override;

		bool has_index() const;

		ACCEPT
	};

	struct ImplStmt : Stmt
	{
		Symbol type_symbol;
		UserDefType *type;
		std::vector<FnType*> methods;

		CLASSOF(Stmt, Impl)

		llvm::Value *emit(EmitCtx &ctx, EmitParams params) override;

		ACCEPT
	};
}

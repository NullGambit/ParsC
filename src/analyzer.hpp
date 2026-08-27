#pragma once
#include <string_view>

#include "comp_eval.hpp"
#include "node.hpp"
#include "parse_ctx.hpp"
#include "stmt.hpp"
#include "type_meta.hpp"
#include "visitor.hpp"
#include "containers/hash_map.hpp"

namespace pars
{
	struct Type;
	struct ParseCtx;
	class AST;

	class Analyzer : public Visitor
	{
	public:
		explicit Analyzer(ParseCtx *parse_ctx);

		Node* visit(CallExpr *expr, VisitCtx ctx) override;
		Node* visit(FnDecl *fn, VisitCtx ctx) override;
		Node* visit(Struct *stmt, VisitCtx ctx) override;
		Node* visit(VarDeclStmt *stmt, VisitCtx ctx) override;
		Node* visit(ImportStmt *stmt, VisitCtx ctx) override;
		Node* visit(ReturnStmt *stmt, VisitCtx ctx) override;
		Node* visit(BlockStmt *stmt, VisitCtx ctx) override;
		Node* visit(AssignmentStmt *stmt, VisitCtx ctx) override;
		Node* visit(IfStmt *stmt, VisitCtx ctx) override;
		Node* visit(CompIfStmt *stmt, VisitCtx ctx) override;
		Node* visit(WhileStmt *stmt, VisitCtx ctx) override;
		Node* visit(ForStmt *stmt, VisitCtx ctx) override;
		Node* visit(AliasType *alias, VisitCtx ctx) override;
		Node* visit(SymbolExpr *expr, VisitCtx ctx) override;
		Node* visit(BinaryExpr *expr, VisitCtx ctx) override;
		Node* visit(UnaryExpr *expr, VisitCtx ctx) override;
		Node* visit(GroupExpr* expr, VisitCtx ctx) override;
		Node* visit(SizeofExpr* expr, VisitCtx ctx) override;
		Node* visit(MemberAccessExpr* expr, VisitCtx ctx) override;
		Node* visit(CastExpr* expr, VisitCtx ctx) override;
		Node* visit(AnonInitExpr* expr, VisitCtx ctx) override;
		Node* visit(NamedExpr* expr, VisitCtx ctx) override;
		Node* visit(AbsExpr* expr, VisitCtx ctx) override;
		Node* visit(PtrOpExpr* expr, VisitCtx ctx) override;
		Node* visit(PackedExpr* expr, VisitCtx ctx) override;
		Node* visit(ArrayLiteralExpr* expr, VisitCtx ctx) override;
		Node* visit(IndexOpExpr* expr, VisitCtx ctx) override;
		Node* visit(StructLiteral* expr, VisitCtx ctx) override;
		Node* visit(SliceExpr* expr, VisitCtx ctx) override;
		Node* visit(UnresolvedSymbol *type, VisitCtx ctx) override;
		Node* visit(Pointer *type, VisitCtx ctx) override;
		Node* visit(BaseArray *type, VisitCtx ctx) override;
		Node* visit(Array *type, VisitCtx ctx) override;
		Node *visit(LiteralExpr *expr, VisitCtx ctx) override;
		Node *visit(FnPtrType *type, VisitCtx ctx) override;

		void analyze(const std::vector<Node*> &nodes);

	private:
		ParseCtx *m_ctx;
		std::vector<FnDecl*> m_function_stack;
		u8 m_expr_depth {};
		CompEval m_comp_eval;

		struct SymbolTask
		{
			Node *node;
			std::function<void(Node *node)> fn;
		};

		HashMap<std::string_view, SymbolTask> m_symbol_tasks;

		FnDecl* get_current_fn();

		Type* resolve_type(TypeMeta &meta, Node *node);

		void add_symbol_task(Type *type, std::string_view symbol, SymbolTask &&task);

		Node* find_symbol(std::string_view name, Token &error_token, ScopeTable *table_override = nullptr);

		template<IsNode T>
		T* find_symbol(std::string_view name, Token &error_token, ScopeTable *table_override = nullptr)
		{
			return dynamic_cast<T*>(find_symbol(name, error_token, table_override));
		}

		Type* get_type(std::string_view name, Token &error_token);

		Expr* visit_expr(Expr *parent, Expr *expr, VisitCtx ctx);
	};
}

#pragma once
#include <string_view>

#include "node.hpp"
#include "parse_ctx.hpp"
#include "visitor.hpp"
#include "containers/hash_map.hpp"

namespace pars
{
	struct Type;
	struct ParseCtx;
	class AST;

	// resolves symbols and does type checking
	// essentially the sanity checker
	// while the ast is just the form checker
	class Analyzer : public Visitor
	{
	public:
		explicit Analyzer(ParseCtx *parse_ctx);

		void visit(CallExpr *expr, VisitCtx ctx) override;
		void visit(FnDecl *fn, VisitCtx ctx) override;
		void visit(VarDeclStmt *stmt, VisitCtx ctx) override;
		void visit(ImportStmt *stmt, VisitCtx ctx) override;
		void visit(ReturnStmt *stmt, VisitCtx ctx) override;
		void visit(AliasType *alias, VisitCtx ctx) override;
		void visit(SymbolExpr *expr, VisitCtx ctx) override;
		void visit(BinaryExpr *expr, VisitCtx ctx) override;
		void visit(UnaryExpr *expr, VisitCtx ctx) override;
		void visit(GroupExpr* expr, VisitCtx ctx) override;
		void visit(SizeofExpr* expr, VisitCtx ctx) override;
		void visit(MemberAccessExpr* expr, VisitCtx ctx) override;
		void visit(CastExpr* expr, VisitCtx ctx) override;
		void visit(AnonInitExpr* expr, VisitCtx ctx) override;

		void analyze(const std::vector<Node*> &nodes);

	private:
		ParseCtx *m_ctx;
		std::vector<FnDecl*> m_function_stack;

		struct SymbolTask
		{
			Node *node;
			std::function<void(Node *node)> fn;
		};

		HashMap<std::string_view, SymbolTask> m_symbol_tasks;

		FnDecl* get_current_fn();

		Type* resolve_type(std::string_view name, Node *node);

		void add_symbol_task(Type *type, std::string_view symbol, SymbolTask &&task);

		Node* find_symbol(std::string_view name, Token &error_token);

		template<IsNode T>
		T* find_symbol(std::string_view name, Token &error_token)
		{
			return dynamic_cast<T*>(find_symbol(name, error_token));
		}
	};
}

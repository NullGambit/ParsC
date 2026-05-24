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

		void visit(CallExpr *expr) override;
		void visit(FnDecl *fn) override;
		void visit(VarDeclStmt *stmt) override;
		void visit(ImportStmt *stmt) override;
		void visit(ReturnStmt *stmt) override;
		void visit(AliasType *alias) override;
		void visit(SymbolExpr *expr) override;
		void visit(BinaryExpr *expr) override;
		void visit(GroupExpr* expr) override;
		void visit(SizeofExpr* expr) override;
		void visit(MemberAccessExpr* expr) override;

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

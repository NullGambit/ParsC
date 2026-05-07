#pragma once
#include <functional>
#include <variant>
#include <vector>

#include "expr.hpp"
#include "lexer.hpp"
#include "scope_table.hpp"
#include "stmt.hpp"
#include "containers/hash_map.hpp"

namespace pars
{
	// a per module parser
	class Parser
	{
	public:

		Parser();

		const std::vector<Node*>& parse(SourceFile source);

		void resolve_symbols();

	private:
		typedef Expr*(Parser::*BinaryRule)();

		struct UnresolvedSymbol
		{
			Token token;
			Node *node;
			std::string_view name;
		};

		Lexer m_lexer {};
		std::vector<Node*> m_nodes;

		std::vector<Node*> *m_target = &m_nodes;

		ScopeTable m_scope_table;

		// any symbol that could not be resolved such as a function or struct that was defined bellow
		// will be added to this table to be resolved later
		std::vector<UnresolvedSymbol> m_unresolved_symbols;

		std::vector<Expr*> m_pending_attributes;

		HashMap<std::string_view, std::function<Expr*(const std::vector<Expr*>&)>> m_builtin_functions;

		Node* declaration();
		Node* statement();
		void declare_to(std::vector<Node*> &nodes);
		Node* parse_import();
		VarDeclStmt* parse_var();
		Node* parse_return();
		Node* parse_println();
		void parse_attributes();
		Symbol get_symbol();
		FnPrototypeStmt* parse_fn_prototype();
		BlockStmt* parse_block();
		ExprFnStmt* parse_expr_fn();

		// expr parsing stuff
		// TODO replace with a pratt parser
		Expr* expression();
		Expr* parse_equality();
		Expr* parse_comparison();
		Expr* parse_term();
		Expr* parse_unary();
		Expr* parse_factor();
		Expr* parse_primary();

		template<class T>
		T* peek()
		{
			if (m_target->empty())
			{
				return nullptr;
			}

			auto *node = m_target->back();

			return dynamic_cast<T*>(node);
		}

		std::vector<Expr*> collect_call_arguments();

		template<TokenType ...T>
		Expr* parse_binary_rule(BinaryRule rule)
		{
			auto *expr = std::invoke(rule, this);

			while ((m_lexer.match(T) || ...))
			{
				auto op = m_lexer.peek_last().lexeme;

				auto right = std::invoke(rule, this);

				auto *bin = new_node<BinaryExpr>();

				bin->left = expr;
				bin->right = right;
				bin->op = op[0];

				expr = bin;
			}

			return expr;
		}
	};
}

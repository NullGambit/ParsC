#pragma once
#include <functional>
#include <variant>
#include <vector>

#include "expr.hpp"
#include "lexer.hpp"
#include "stmt.hpp"
#include "containers/hash_map.hpp"

namespace pars
{
    constexpr auto FN_PROTOTYPE_NEEDS_SYMBOL = true;
    constexpr auto FN_PROTOTYPE_NEEDS_BODY = true;
    constexpr auto FN_PROTOTYPE_ANON = false;
    constexpr auto FN_PROTOTYPE_BODYLESS = false;

	// a per module parser
	class Parser
	{
	public:

		Parser();

		const std::vector<Node*>& parse(SourceFile source);

	private:
		typedef Expr*(Parser::*BinaryRule)();

		Lexer m_lexer {};
		std::vector<Node*> m_nodes;
		HashMap<std::string_view, std::function<void(const std::vector<Expr*>&)>> m_builtin_functions;

		Node* declaration();
		void declare_to(std::vector<Node*> &nodes);
		Node* parse_import();
		Stmt* parse_fn();
		Node* parse_var();
		Node* parse_return();
		Node* parse_println();
		FnPrototype parse_fn_prototype();

		// expr parsing stuff
		// TODO replace with a pratt parser
		Expr* expression();
		Expr* parse_equality();
		Expr* parse_comparison();
		Expr* parse_term();
		Expr* parse_unary();
		Expr* parse_factor();
		Expr* parse_primary();

		std::vector<Expr*> collect_call_arguments();

		template<TokenType ...T>
		Expr* parse_binary_rule(BinaryRule rule)
		{
			auto *expr = std::invoke(rule, this);

			while ((m_lexer.match(T) || ...))
			{
				auto op = m_lexer.peak_last().lexeme;

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

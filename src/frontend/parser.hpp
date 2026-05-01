#pragma once
#include <variant>
#include <vector>

#include "expr.hpp"
#include "lexer.hpp"
#include "stmt.hpp"

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

		const std::vector<Node*>& parse(SourceFile source);

	private:
		Lexer m_lexer {};
		std::vector<Node*> m_nodes;

		Node* decleration();
		Node* parse_import();
		Stmt* parse_fn();
		Node* parse_var();
		FnPrototype parse_fn_prototype();
		Expr* expression();
	};
}

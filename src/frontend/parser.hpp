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

		const std::vector<size_t>& parse(SourceFile source);

	private:
		Lexer m_lexer;
		std::vector<size_t> m_stmts;
		std::vector<Expression> m_exprs;

		size_t decleration();
		size_t parse_import();
		size_t parse_fn();
		size_t parse_var();
		FnPrototype parse_fn_prototype();
		size_t expression();
	};
}

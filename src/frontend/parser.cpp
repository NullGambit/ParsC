#include "parser.hpp"

#include "frontend/token.hpp"
#include "util/fmt.hpp"

using enum pars::TokenType;

const std::vector<pars::Node*>& pars::Parser::parse(SourceFile source)
{
	m_lexer.set_source(source);

	while (m_lexer.has_next())
	{
		auto *node = decleration();
		m_nodes.emplace_back(node);

	}

	return m_nodes;
}

pars::Node* pars::Parser::decleration()
{
	if (m_lexer.match(Import))
	{
		return parse_import();
	}
	if (m_lexer.match(Fn))
	{
		return parse_fn();
	}
	if (m_lexer.match(Var))
	{
		return parse_var();
	}

	return expression();
}

pars::Node* pars::Parser::parse_import()
{
	auto *stmt = new_node<ImportStmt>();

	if (m_lexer.match_next(Equal))
	{
		stmt->alias = m_lexer.peak_last().lexeme;
	}

	while (m_lexer.match(Identifier))
	{
		stmt->path.push_back(m_lexer.peak_last().lexeme);

		if (!m_lexer.match(Dot))
		{
			break;
		}
	}

	return stmt;
}

pars::Stmt* pars::Parser::parse_fn()
{
	auto *stmt = new_node<FnStmt>();

	stmt->body = {};

	stmt->symbol = m_lexer.expect(Identifier).lexeme;

	stmt->prototype = parse_fn_prototype();

	if (m_lexer.match(Arrow))
	{
		stmt->body.emplace_back(expression());
	}
	else
	{
		m_lexer.expect(LeftBrace);

		while (!m_lexer.peak(RightBrace))
		{
			stmt->body.emplace_back(decleration());
		}

		m_lexer.expect(RightBrace);
	}

	return stmt;
}

pars::Node* pars::Parser::parse_var()
{
	auto *stmt = new_node<VarStmt>();

	stmt->symbol = m_lexer.expect(Identifier).lexeme;

	auto was_typed = false;
	auto initialized = false;

	if (m_lexer.match(Colon))
	{
		stmt->type = m_lexer.expect(Identifier).lexeme;
		was_typed = true;
	}

	if (m_lexer.match(Equal))
	{
		stmt->initializer = expression();
		initialized = true;
	}

	if (!was_typed && !initialized)
	{
		throw Token{.type = Error, .lexeme = "Cannot infer type"};
	}

	return stmt;
}

pars::FnPrototype pars::Parser::parse_fn_prototype()
{
	m_lexer.expect(LeftParen);

	FnPrototype prototype;
// fn main(a: i32) {
// fn main() {
	fmt::println("{}", m_lexer.peak().lexeme);
	while (!m_lexer.peak(RightParen))
	{
		TypedSymbol symbol;

		symbol.name = m_lexer.expect(Identifier).lexeme;

		m_lexer.expect(Colon);

		symbol.type = m_lexer.expect(Identifier).lexeme;

		prototype.parameters.push_back(symbol);

		if (!m_lexer.peak(RightParen))
		{
			m_lexer.expect(Comma);
		}
	}

	m_lexer.expect(RightParen);

	if (m_lexer.match(Colon))
	{
		prototype.return_type = m_lexer.expect(Identifier).lexeme;
	}

	return prototype;
}

pars::Expr* pars::Parser::expression()
{
	return {};
}


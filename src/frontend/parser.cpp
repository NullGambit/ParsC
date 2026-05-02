#include "parser.hpp"

#include <charconv>

#include "frontend/token.hpp"
#include "util/fmt.hpp"

using enum pars::TokenType;

pars::Parser::Parser()
{
	auto print_args = [](const std::vector<Expr*> &args)
	{
		for (auto *arg : args)
		{
			auto *literal = dynamic_cast<LiteralExpr*>(arg);

			if (literal != nullptr && literal->value.index() == 2)
			{
				auto s = std::get<std::string_view>(literal->value);
				fmt::print("{}", s);
			}
		}

		fmt::println("");
	};

	m_builtin_functions["pragma"] = print_args;
	m_builtin_functions["panic"] = [print_args](const auto &args)
	{
		print_args(args);
		std::exit(-1);
	};
}

const std::vector<pars::Node*>& pars::Parser::parse(SourceFile source)
{
	m_lexer.set_source(source);

	while (m_lexer.has_next())
	{
		declare_to(m_nodes);
	}

	return m_nodes;
}

pars::Node* pars::Parser::declaration()
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
	if (m_lexer.match(Return))
	{
		return parse_return();
	}
	if (m_lexer.match(Println))
	{
		return parse_println();
	}

	return expression();
}

void pars::Parser::declare_to(std::vector<Node *> &nodes)
{
	auto *node = declaration();

	[[likely]]
	if (node != nullptr)
	{
		nodes.emplace_back(node);
	}
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
			declare_to(stmt->body);
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

pars::Node * pars::Parser::parse_return()
{
	auto stmt = new_node<ReturnStmt>();

	stmt->expr = expression();

	return stmt;
}

pars::Node * pars::Parser::parse_println()
{
	auto *stmt = new_node<PrintlnStmt>();

	stmt->expr = expression();

	return stmt;
}

pars::FnPrototype pars::Parser::parse_fn_prototype()
{
	m_lexer.expect(LeftParen);

	FnPrototype prototype;

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

pars::Expr* pars::Parser::parse_primary()
{
	if (m_lexer.match(Dollar))
	{
		auto identifier = m_lexer.expect(Identifier).lexeme;

		auto iter = m_builtin_functions.find(identifier);

		if (iter != m_builtin_functions.end())
		{
			if (m_lexer.match(LeftParen))
			{
				CallExpr expr;

				expr.symbol = identifier;

				expr.arguments = collect_call_arguments();

				m_lexer.expect(RightParen);

				iter->second(expr.arguments);
			}
		}

		return nullptr;
	}

	if (m_lexer.match(LeftParen))
	{
		auto *expr = expression();

		m_lexer.expect(RightParen);

		auto *group = new_node<GroupExpr>();

		group->expr = expr;

		return group;
	}
	if (m_lexer.match(Identifier))
	{
		auto identifier = m_lexer.peak_last().lexeme;

		if (m_lexer.match(LeftParen))
		{
			auto *expr = new_node<CallExpr>();

			expr->symbol = identifier;

			expr->arguments = collect_call_arguments();

			m_lexer.expect(RightParen);

			return expr;
		}

		auto *expr = new_node<SymbolExpr>();

		expr->symbol = identifier;

		return expr;
	}

	auto *literal = new_node<LiteralExpr>();

	if (m_lexer.match(True))
	{
		literal->value = true;
	}
	if (m_lexer.match(False))
	{
		literal->value = false;
	}
	if (m_lexer.match(IntegerLiteral))
	{
		auto lexeme = m_lexer.peak_last().lexeme;
		i64 n = 0;
		std::from_chars(lexeme.begin(), lexeme.end(), n);
		literal->value = n;
	}
	if (m_lexer.match(DecimalLiteral))
	{
		auto lexeme = m_lexer.peak_last().lexeme;
		f32 n = 0;
		std::from_chars(lexeme.begin(), lexeme.end(), n);
		literal->value = n;
	}
	if (m_lexer.match(StringLiteral))
	{
		literal->value = m_lexer.peak_last().lexeme;
	}

	return literal;
}

std::vector<pars::Expr*> pars::Parser::collect_call_arguments()
{
	std::vector<Expr*> arguments;

	while (true)
	{
		arguments.emplace_back(expression());

		if (!m_lexer.match(Comma))
		{
			break;
		}
	}

	return arguments;
}

pars::Expr* pars::Parser::parse_unary()
{
	if (m_lexer.match(Bang) || m_lexer.match(Minus))
	{
		auto op = m_lexer.peak_last();

		auto expr = parse_unary();

		auto *unary = new_node<UnaryExpr>();

		unary->op = op.lexeme[0];
		unary->right = expr;

		return unary;
	}

	return parse_primary();
}

pars::Expr* pars::Parser::parse_factor()
{
	return parse_binary_rule<Star, ForwardSlash>(&Parser::parse_unary);
}

pars::Expr* pars::Parser::parse_term()
{
	return parse_binary_rule<Plus, Minus>(&Parser::parse_factor);
}

pars::Expr* pars::Parser::parse_comparison()
{
	return parse_binary_rule<Less, LessEqual, Greater, GreaterEqual>(&Parser::parse_term);
}

pars::Expr* pars::Parser::parse_equality()
{
	return parse_binary_rule<EqualEqual, BangEqual>(&Parser::parse_comparison);
}

pars::Expr* pars::Parser::expression()
{
	return parse_equality();
}


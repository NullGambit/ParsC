#include "parser.hpp"

#include <charconv>

#include "frontend_error.hpp"
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

		return nullptr;
	};

	m_builtin_functions["pragma"] = print_args;
	m_builtin_functions["panic"] = [print_args](const auto &args) -> Expr*
	{
		print_args(args);
		std::exit(-1);
	};

	// m_builtin_functions["has_attr"] = [](const std::vector<Expr*> &args)
	// {
	//
	// };
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
	if (m_lexer.match(At))
	{
		parse_attributes();
	}

	if (m_lexer.match(Fn))
	{
		return parse_fn();
	}
	if (m_lexer.match(Var))
	{
		return parse_var();
	}

	// attributes not bound to any declaration symbol
	// so discard them
	m_pending_attributes.clear();

	return statement();
}

pars::Node * pars::Parser::statement()
{
	if (m_lexer.match(Import))
	{
		return parse_import();
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

	stmt->symbol = get_symbol();

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

	stmt->symbol = get_symbol();

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
		throw FrontendError(m_lexer.peak_last(), "Cannot infer type", stmt);
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

void pars::Parser::parse_attributes()
{
	if (!m_lexer.match(LeftBracket))
	{
		m_pending_attributes.emplace_back(expression());
	}
	else
	{
		while (true)
		{
			m_pending_attributes.emplace_back(expression());

			if (!m_lexer.match(Comma))
			{
				break;
			}
		}

		m_lexer.expect(RightBracket);
	}
}

pars::Symbol pars::Parser::get_symbol()
{
	auto symbol = Symbol
	{
		.name = m_lexer.expect(Identifier).lexeme
	};

	if (!m_pending_attributes.empty())
	{
		symbol.attribute_id = get_attribute_id();
		symbol.attribute_count = m_pending_attributes.size();

		set_attributes(m_pending_attributes);

		m_pending_attributes.clear();
	}

	return symbol;
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

				return iter->second(expr.arguments);
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


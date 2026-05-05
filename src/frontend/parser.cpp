#include "parser.hpp"

#include <charconv>

#include "frontend_error.hpp"
#include "frontend/token.hpp"
#include "util/fmt.hpp"

using enum pars::TokenType;

pars::Parser::Parser()
{
	m_scope_table.resize(6);

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

void pars::Parser::resolve_symbols()
{
	for (auto unresolved : m_unresolved_symbols)
	{
		auto *symbol = find_symbol(unresolved.name);

		if (symbol == nullptr)
		{
			throw FrontendError(unresolved.token, "Symbol not defined", unresolved.node);
		}
	}
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
	if (m_lexer.match(Var) || m_lexer.match(Const))
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

void pars::Parser::parse_scope(std::vector<Node*> &nodes)
{
	m_lexer.expect(LeftBrace);

	m_scope++;

	if (m_scope >= m_scope_table.size())
	{
		m_scope_table.emplace_back();
	}

	while (!m_lexer.peek(RightBrace))
	{
		declare_to(nodes);
	}

	m_scope_table[m_scope].clear();

	m_scope--;

	m_lexer.expect(RightBrace);
}

pars::Node* pars::Parser::parse_import()
{
	auto *stmt = new_node<ImportStmt>();

	if (m_lexer.match_next(Equal))
	{
		stmt->alias = m_lexer.peek_last().lexeme;
	}

	while (m_lexer.match(Identifier))
	{
		stmt->path.push_back(m_lexer.peek_last().lexeme);

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

	stmt->symbol = get_symbol();

	add_to_scope(stmt->symbol, stmt);

	stmt->prototype = parse_fn_prototype();

	for (auto *param : stmt->prototype.parameters)
	{
		add_to_scope(param->symbol.name, param, m_scope + 1);
	}

	if (m_lexer.match(Arrow))
	{
		m_scope++;
		stmt->body.emplace_back(expression());
		m_scope--;
	}
	else
	{
		auto &old_target = m_target;

		m_target = stmt->body;

		parse_scope(stmt->body);

		m_target = old_target;
	}

	return stmt;
}

pars::VarDeclStmt* pars::Parser::parse_var()
{
	auto *stmt = new_node<VarDeclStmt>();

	if (m_lexer.peek_last(Const))
	{
		stmt->flags |= VarFlags::Const;
	}

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
		throw FrontendError(m_lexer.peek_last(), "Cannot infer type", stmt);
	}

	add_to_scope(stmt->symbol, stmt);

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

pars::FnPrototypeStmt pars::Parser::parse_fn_prototype()
{
	FnPrototypeStmt prototype;

	// TODO actually handle externing inside declaration
	prototype.is_extern = m_lexer.peek_last(Extern);

	m_lexer.expect(LeftParen);

	while (!m_lexer.peek(RightParen))
	{
		prototype.parameters.push_back(parse_var());

		if (!m_lexer.peek(RightParen))
		{
			m_lexer.expect(Comma);
		}
	}

	m_lexer.expect(RightParen);

	if (m_lexer.match(Colon))
	{
		prototype.return_type = m_lexer.expect(Identifier).lexeme;
	}
	else
	{
		prototype.return_type = "void";
	}

	return prototype;
}

pars::Parser::Scope & pars::Parser::get_current_scope()
{
	if (m_scope >= m_scope_table.size())
	{
		return m_scope_table.emplace_back();
	}

	return m_scope_table[m_scope];
}

void pars::Parser::add_to_scope(Symbol symbol, Node *node, u32 level)
{
	add_to_scope(symbol.name, node, level);
}

void pars::Parser::add_to_scope(std::string_view name, Node *node, u32 level)
{
	Scope *scope;

	if (level != UINT32_MAX)
	{
		if (level >= m_scope_table.size())
		{
			for (u32 i = 0; i < level; i++)
			{
				m_scope_table.emplace_back();
			}
		}

		scope = &m_scope_table[level];
	}
	else
	{
		scope = &get_current_scope();
	}

	scope->emplace(name, node);
}

pars::Node * pars::Parser::find_symbol(std::string_view name)
{
	// TODO: allow for parameterized up to down or down to up scope checking
	for (auto i = 0; i <= m_scope; i++)
	{
		auto &scope = m_scope_table[i];
		auto iter = scope.find(name);

		if (iter != scope.end())
		{
			return iter->second;
		}
	}

	return nullptr;
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
		auto identifier = m_lexer.peek_last().lexeme;

		auto symbol = find_symbol(identifier);

		UnresolvedSymbol unresolved_symbol
		{
			.token = m_lexer.peek_last(),
			.name = identifier,
		};

		auto resolved_symbol = false;

		if (symbol != nullptr)
		{
			resolved_symbol = true;
		}

		Expr *result;

		if (m_lexer.match(LeftParen))
		{
			auto *expr = new_node<CallExpr>();

			expr->symbol = identifier;

			expr->arguments = collect_call_arguments();

			m_lexer.expect(RightParen);

			result = expr;
		}
		else
		{
			auto *expr = new_node<SymbolExpr>();

			expr->symbol = identifier;

			result = expr;
		}

		if (!resolved_symbol)
		{
			unresolved_symbol.node = result;
			m_unresolved_symbols.emplace_back(unresolved_symbol);
		}

		return result;
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
		auto lexeme = m_lexer.peek_last().lexeme;
		i64 n = 0;
		std::from_chars(lexeme.begin(), lexeme.end(), n);
		literal->value = n;
	}
	if (m_lexer.match(DecimalLiteral))
	{
		auto lexeme = m_lexer.peek_last().lexeme;
		f32 n = 0;
		std::from_chars(lexeme.begin(), lexeme.end(), n);
		literal->value = n;
	}
	if (m_lexer.match(StringLiteral))
	{
		literal->value = m_lexer.peek_last().lexeme;
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
		auto op = m_lexer.peek_last();

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


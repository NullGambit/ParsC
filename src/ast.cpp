#include "ast.hpp"

#include <charconv>
#include <filesystem>

#include "frontend_error.hpp"
#include "module.hpp"
#include "module_manager.hpp"
#include "parse_ctx.hpp"
#include "token.hpp"
#include "type.hpp"
#include "util/fmt.hpp"

using enum pars::TokenType;

// macros are dogshit but seems like a better option than using std::function
// which allocates. ideally would write my own that holds a stack based buffer for small captures but thats for future me to worry about
#define COLLECT_COMMA_SEP(start, end, fn)   \
do											\
{											\
	m_lexer.expect((start));				\
											\
	while (!m_lexer.peek((end)))			\
	{										\
		(fn);								\
											\
		if (!m_lexer.peek((end)))			\
		{									\
			m_lexer.expect(Comma);			\
		}									\
	}										\
											\
	m_lexer.expect((end));					\
}											\
while (false)								\

pars::AST::AST()
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

const std::vector<pars::Node*>& pars::AST::parse(ParseCtx *ctx)
{
	m_ctx = ctx;

	m_lexer.set_source(ctx->source_file);

	m_target = &m_nodes;

	// only need to record global symbols right now
	m_ctx->scope_table.set_lock(0);

	while (m_lexer.has_next() && !m_lexer.match_next(Eof))
	{
		declare_to(m_nodes);
	}

	m_ctx->scope_table.set_lock(ScopeTable::UNLOCKED_LEVEL);

	return m_nodes;
}

pars::ParseCtx * pars::AST::get_ctx() const
{
	return m_ctx;
}

u32 pars::AST::get_file_id() const
{
	return m_ctx->source_file.id;
}

pars::Node* pars::AST::declaration()
{
	if (m_lexer.match(At))
	{
		parse_attributes();
	}

	if (m_lexer.match(LeftBrace))
	{
		while (!m_lexer.peek(RightBrace))
		{
			declare_to(*m_target);
		}

		m_lexer.expect(RightBrace);
	}

	if (m_lexer.match(Fn))
	{
		return parse_fn({});
	}
	if (m_lexer.match(Var) || m_lexer.match(Const))
	{
		return parse_var();
	}
	if (m_lexer.match(Alias))
	{
		return parse_alias();
	}

	// attributes not bound to any declaration symbol
	// so discard them
	m_pending_attributes.clear();

	return statement();
}

pars::Node * pars::AST::statement()
{
	if (m_lexer.match(Import))
	{
		return parse_import();
	}
	if (m_lexer.match(Return))
	{
		return parse_return();
	}

	return expression();
}

void pars::AST::declare_to(std::vector<Node *> &nodes)
{
	auto *node = declaration();

	[[likely]]
	if (node != nullptr)
	{
		nodes.emplace_back(node);
	}
}

pars::Node* pars::AST::parse_import()
{
	// TODO: a multithreading optimization that i can make here is
	// since i know i might need a module with a specific path later.
	// while this module is parsing tell the module manager to prewarm this imports module
	// and by prewarm i mean fully parse and compile
	auto *stmt = new_node<ImportStmt>();

	if (m_lexer.match_next(Equal))
	{
		stmt->alias = m_lexer.peek_last().lexeme;
		m_lexer.advance();
	}

	while (m_lexer.match(Identifier))
	{
		stmt->path /= m_lexer.peek_last().lexeme;

		if (!m_lexer.match(Dot))
		{
			break;
		}
	}

	if (m_lexer.match(Colon))
	{
		while (m_lexer.match(Identifier))
		{
			auto symbol_name = m_lexer.peek_last().lexeme;
			auto import_name = symbol_name;

			if (m_lexer.match(Equal))
			{
				symbol_name = m_lexer.expect(Identifier).lexeme;
			}

			stmt->selective_imports.emplace_back(symbol_name, import_name);

			if (!m_lexer.match(Comma))
			{
				break;
			}
		}
	}

	return stmt;
}

pars::VarDeclStmt* pars::AST::parse_var()
{
	auto *stmt = new_node<VarDeclStmt>();

	if (m_lexer.peek_last(Const))
	{
		stmt->flags |= VarFlags::Const;
	}

	stmt->symbol = get_symbol();

	if (m_lexer.match(Colon))
	{
		stmt->type_name = m_lexer.expect(Identifier).lexeme;
	}

	if (m_lexer.match(Equal))
	{
		stmt->initializer = expression();

	}

	if (stmt->type_name.empty() && stmt->initializer == nullptr)
	{
		throw FrontendError(m_lexer.peek_last(), "Variable without initializer must be explicitly typed", stmt);
	}

	m_ctx->scope_table.add_to_scope(stmt->symbol, stmt);

	return stmt;
}

pars::Node* pars::AST::parse_return()
{
	auto *fn = get_current_fn();

	if (fn == nullptr)
	{
		throw FrontendError{m_lexer.peek_last(), "cannot return outside of function"};
	}

	auto *stmt = new_node<ReturnStmt>();

	if (fn->signature.return_type_name != "void")
	{
		stmt->expr = expression();
	}

	return stmt;
}

void pars::AST::parse_attributes()
{
	// TODO: instead of calling expression have a finer grained way to get attributes
	// currently this will not report errors if you randomly use a keyword somewhere
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

pars::Symbol pars::AST::get_symbol()
{
	auto symbol = Symbol
	{
		.name = m_lexer.expect(Identifier).lexeme
	};

	if (!m_pending_attributes.empty())
	{
		symbol.attribute_count = m_pending_attributes.size();
		symbol.attribute_id = set_attributes(m_pending_attributes);

		m_pending_attributes.clear();
	}

	return symbol;
}

pars::FnSignature pars::AST::parse_fn_signature()
{
	FnSignature signature;

	COLLECT_COMMA_SEP(LeftParen, RightParen,
		signature.parameters.push_back(parse_var()));

	if (m_lexer.match(Colon))
	{
		signature.return_type_name = m_lexer.expect(Identifier).lexeme;
	}
	else
	{
		signature.return_type_name = "void";
	}

	return signature;
}

pars::FnDecl* pars::AST::parse_fn(FnFlags flags)
{
	auto *fn = new_node<FnDecl>();

	fn->flags = flags;
	fn->symbol = get_symbol();

	if (has_keyword_attribute(fn->symbol, Extern))
	{
		fn->flags |= FnFlags::Extern;
	}
	if (has_keyword_attribute(fn->symbol, Private))
	{
		fn->flags |= FnFlags::Private;
	}

	m_ctx->scope_table.add_to_scope(fn->symbol, fn, !has_flag(fn->flags, FnFlags::Private));

	fn->signature = parse_fn_signature();

	m_function_stack.emplace_back(fn);

	auto *old_target = m_target;

	m_target = &fn->body;

	if (m_lexer.match(Arrow))
	{
		fn->flags |= FnFlags::Inline | FnFlags::ArrowFn;

		// auto *ret = new_node<ReturnStmt>();
		//
		// ret->expr = expression();

		fn->body.emplace_back(expression());
	}
	else if (m_lexer.match(LeftBrace))
	{
		while (!m_lexer.peek(RightBrace))
		{
			declare_to(fn->body);
		}

		m_lexer.expect(RightBrace);
	}
	else if (!has_flag(fn->flags, FnFlags::Extern))
	{
		throw FrontendError{m_lexer.peek(), "non extern function must have a body", fn};
	}

	m_target = old_target;

	m_function_stack.pop_back();

	return fn;
}

pars::AliasType * pars::AST::parse_alias()
{
	auto stmt = new_node<AliasType>();

	stmt->symbol = get_symbol();

	m_lexer.expect(Equal);

	stmt->is_distinct = m_lexer.match(Distinct);

	auto *pending = new_node<PendingType>();

	pending->symbol = m_lexer.expect(Identifier).lexeme;

	stmt->type = pending;

	m_ctx->scope_table.add_to_scope(stmt->symbol, stmt);

	return stmt;
}

pars::Expr* pars::AST::parse_primary()
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

		group->inner = expr;
		group->type = expr->type;

		return group;
	}
	if (m_lexer.match(Identifier))
	{
		const auto identifier = m_lexer.peek_last().lexeme;

		if (m_lexer.match(LeftParen))
		{
			auto *expr = new_node<CallExpr>();

			expr->symbol = identifier;

			expr->arguments = collect_call_arguments();

			m_lexer.expect(RightParen);

			return expr;
		}
		if (m_lexer.match(Dot))
		{
			auto *expr = new_node<MemberAccessExpr>();

			expr->target_symbol = identifier;
			expr->accessor = expression();

			return expr;
		}

		auto *expr = new_node<SymbolExpr>();

		expr->symbol = identifier;

		return expr;
	}
	if (m_lexer.match(Sizeof))
	{
		auto *expr = new_node<SizeofExpr>();

		expr->type = const_cast<Integer*>(&I32Type);

		expr->expr = expression();

		return expr;
	}
	if (m_lexer.match(Cast))
	{
		m_lexer.expect(LeftParen);

		auto *expr = new_node<CastExpr>();

		expr->type_expr = expression();

		m_lexer.expect(RightParen);

		expr->target = parse_unary();

		return expr;
	}
	if (m_lexer.peek().type > _AttributeKeywordStart && m_lexer.peek().type < _AttributeKeywordEnd)
	{
		m_lexer.advance();

		return new_node<KeywordExpr>();
	}

	auto *literal = new_node<LiteralExpr>();

	if (m_lexer.match(True) || m_lexer.match(False))
	{
		literal->value = m_lexer.peek_last(True);
		literal->type = const_cast<Bool*>(&BoolType);
	}
	else if (m_lexer.match(IntegerLiteral))
	{
		auto lexeme = m_lexer.peek_last().lexeme;
		i32 n = 0;
		std::from_chars(lexeme.begin(), lexeme.end(), n);
		literal->type = const_cast<Integer*>(&I32Type);
		literal->value = n;
	}
	else if (m_lexer.match(DecimalLiteral))
	{
		auto lexeme = m_lexer.peek_last().lexeme;
		f32 n = 0;
		std::from_chars(lexeme.begin(), lexeme.end(), n);
		literal->type = const_cast<Float*>(&F32Type);
		literal->value = n;
	}
	else if (m_lexer.match(StringLiteral))
	{
		literal->value = m_lexer.peek_last().lexeme;
		literal->type = const_cast<Str*>(&StrType);
	}
	else if (m_lexer.match(CharLiteral))
	{
		literal->value = m_lexer.peek_last().lexeme[0];
		literal->type = const_cast<Char*>(&CharType);
	}
	else
	{
		throw FrontendError{m_lexer.peek_last(),
			fmt::format("Expected expression but got {}", m_lexer.peek().lexeme), literal};
	}

	return literal;
}

pars::FnDecl* pars::AST::get_current_fn()
{
	if (m_function_stack.empty())
	{
		return nullptr;
	}

	return m_function_stack.back();
}

bool pars::AST::followed_by_body()
{
	return m_lexer.peek(LeftBrace) || m_lexer.peek(Arrow);
}

std::vector<pars::Expr*> pars::AST::collect_call_arguments()
{
	std::vector<Expr*> arguments;

	while (!m_lexer.peek(RightParen))
	{
		arguments.emplace_back(expression());

		if (!m_lexer.match(Comma))
		{
			break;
		}
	}

	return arguments;
}

pars::Expr* pars::AST::parse_unary()
{
	if (m_lexer.match(Bang) || m_lexer.match(Minus))
	{
		auto op = m_lexer.peek_last();

		auto *expr = parse_unary();

		auto *unary = new_node<UnaryExpr>();

		unary->op = op.lexeme[0];
		unary->right = expr;

		return unary;
	}

	return parse_primary();
}

pars::Expr * pars::AST::parse_exp()
{
	return parse_binary_rule<StarStar>(&AST::parse_unary);
}

pars::Expr* pars::AST::parse_factor()
{
	return parse_binary_rule<Star, ForwardSlash, Percent>(&AST::parse_exp);
}

pars::Expr* pars::AST::parse_term()
{
	return parse_binary_rule<Plus, Minus>(&AST::parse_factor);
}

pars::Expr* pars::AST::parse_comparison()
{
	return parse_binary_rule<Less, LessEqual, Greater, GreaterEqual>(&AST::parse_term);
}

pars::Expr* pars::AST::parse_equality()
{
	return parse_binary_rule<EqualEqual, BangEqual>(&AST::parse_comparison);
}

pars::Expr * pars::AST::parse_or()
{
	return parse_binary_rule<Or>(&AST::parse_equality);
}

pars::Expr * pars::AST::parse_and()
{
	return parse_binary_rule<And>(&AST::parse_or);
}

pars::Expr* pars::AST::expression()
{
	return parse_and();
}


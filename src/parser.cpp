#include "parser.hpp"

#include <charconv>

#include "frontend_error.hpp"
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

	auto add_type_to_scope = [&](const Type *type)
	{
		m_scope_table.add_to_scope(type->get_type_name(), const_cast<Type*>(type));
	};

	add_type_to_scope(&I8Type);
	add_type_to_scope(&U8Type);
	add_type_to_scope(&I16Type);
	add_type_to_scope(&U16Type);
	add_type_to_scope(&I32Type);
	add_type_to_scope(&U32Type);
	add_type_to_scope(&I64Type);
	add_type_to_scope(&U64Type);
	add_type_to_scope(&BoolType);
	add_type_to_scope(&StrType);
	add_type_to_scope(&CharType);
	add_type_to_scope(&UCharType);
	add_type_to_scope(&F32Type);
	add_type_to_scope(&F64Type);
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
	for (auto [name, unresolved_list] : m_pending_symbols)
	{
		auto has_symbol = m_scope_table.has_symbol(name);

		if (!has_symbol)
		{
			auto [node, task] = unresolved_list.front();
			throw FrontendError(node->token, fmt::format("Symbol '{}' not defined", name), node);
		}

		for (auto [node, task] : unresolved_list)
		{
			std::invoke(task, this, node);
		}
	}

	for (auto [node, task] : m_post_resolved_tasks)
	{
		std::invoke(task, this, node);
	}
}

pars::Node* pars::Parser::declaration()
{
	if (m_lexer.match(At))
	{
		parse_attributes();
	}

	if (m_lexer.match(Extern))
	{
		if (m_lexer.match(Identifier) && m_lexer.peek_last().lexeme != "C")
		{
			throw FrontendError{m_lexer.peek_last(), "unsupported extern type found"};
		}

		m_lexer.expect(Fn);

		auto *prototype = parse_fn_prototype();

		prototype->is_extern = true;

		if (followed_by_body())
		{
			throw FrontendError{m_lexer.peek(), "Extern function must not have a body", prototype};
		}

		return prototype;
	}
	if (m_lexer.match(Fn))
	{
		auto *prototype = parse_fn_prototype();

		if (!followed_by_body())
		{
			throw FrontendError{m_lexer.peek_last(), "Non extern function must have a body", prototype};
		}

		return prototype;
	}
	if (m_lexer.match(LeftBrace))
	{
		return parse_block();
	}
	if (peek<FnPrototypeStmt>() && m_lexer.match(Arrow))
	{
		return parse_expr_fn();
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

pars::VarDeclStmt* pars::Parser::parse_var()
{
	auto *stmt = new_node<VarDeclStmt>();

	if (m_lexer.peek_last(Const))
	{
		stmt->flags |= VarFlags::Const;
	}

	stmt->symbol = get_symbol();

	if (m_lexer.match(Colon))
	{
		stmt->type = resolve_type();
	}

	if (m_lexer.match(Equal))
	{
		stmt->initializer = expression();

		if (auto *pending = dynamic_cast<PendingType*>(stmt->initializer->type))
		{
			add_symbol_resolved_task(pending->symbol, stmt, &Parser::patch_var_init_type);
		}
		else if (stmt->type != nullptr && !stmt->type->is_equal(stmt->initializer->type))
		{
			throw FrontendError
			{
				m_lexer.peek_last(),
				fmt::format
				(
					"cannot initialize variable {} of type {} with type {}",
					stmt->symbol.name, stmt->type->get_type_name(), stmt->initializer->type->get_type_name()
				)
			};
		}
		else
		{
			stmt->type = stmt->initializer->type;
		}
	}


	if (stmt->type == nullptr && stmt->initializer == nullptr)
	{
		throw FrontendError(m_lexer.peek_last(), "Variable without initializer must be explicitly typed", stmt);
	}

	m_scope_table.add_to_scope(stmt->symbol, stmt);

	return stmt;
}

pars::Node * pars::Parser::parse_return()
{
	auto *fn = get_current_fn();

	if (fn == nullptr)
	{
		throw FrontendError{m_lexer.peek_last(), "cannot return outside of function"};
	}

	auto stmt = new_node<ReturnStmt>();

	if (!fn->return_type->is_equal(&VoidType))
	{
		stmt->expr = expression();
	}

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



pars::FnPrototypeStmt* pars::Parser::parse_fn_prototype()
{
	auto *prototype = new_node<FnPrototypeStmt>();

	prototype->symbol = get_symbol();

	m_scope_table.add_to_scope(prototype->symbol, prototype);

	COLLECT_COMMA_SEP(LeftParen, RightParen,
		prototype->parameters.push_back(parse_var()));

	if (m_lexer.match(Colon))
	{
		prototype->return_type = resolve_type();
	}
	else
	{
		prototype->return_type = const_cast<Void*>(&VoidType);
	}

	return prototype;
}

pars::BlockStmt* pars::Parser::parse_block()
{
	auto *stmt = new_node<BlockStmt>();

	auto scope = m_scope_table.new_scope();

	auto pop_stack = false;

	// we must be inside a function block so add params to scope
	if (auto *prototype = peek<FnPrototypeStmt>(); prototype)
	{
		stmt->owner = prototype;

		for (auto *param : prototype->parameters)
		{
			m_scope_table.add_to_scope(param->symbol.name, param);
		}

		m_function_stack.emplace_back(prototype);
		pop_stack = true;
	}

	auto *old_target = m_target;

	m_target = &stmt->body;

	while (!m_lexer.peek(RightBrace))
	{
		declare_to(stmt->body);
	}

	m_target = old_target;

	m_lexer.expect(RightBrace);

	if (pop_stack)
	{
		m_function_stack.pop_back();
	}

	return stmt;
}

pars::ExprFnStmt * pars::Parser::parse_expr_fn()
{
	auto *stmt = new_node<ExprFnStmt>();

	stmt->owner = peek<FnPrototypeStmt>();
	stmt->expr = expression();

	if (!stmt->owner->return_type->is_equal(&VoidType) && !stmt->owner->return_type->is_equal(stmt->expr->type))
	{
		throw FrontendError{m_lexer.peek_last(), "return type does not match"};
	}

	stmt->owner->return_type = stmt->expr->type;

	return stmt;
}

pars::AliasType * pars::Parser::parse_alias()
{
	auto stmt = new_node<AliasType>();

	stmt->symbol = get_symbol();

	m_lexer.expect(Equal);

	stmt->is_distinct = m_lexer.match(Distinct);

	stmt->type = resolve_type();

	m_scope_table.add_to_scope(stmt->symbol, stmt);

	return stmt;
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
		group->type = expr->type;

		return group;
	}
	if (m_lexer.match(Identifier))
	{
		auto identifier = m_lexer.peek_last().lexeme;

		auto symbol = m_scope_table.find_symbol(identifier);

		if (auto *type = dynamic_cast<Type*>(symbol))
		{
			auto *expr = new_node<TypeExpr>();

			expr->type = type;

			return expr;
		}

		if (m_lexer.match(LeftParen))
		{
			auto *expr = new_node<CallExpr>();

			expr->symbol = identifier;

			// TODO should be replaced with a check to see if its an object that can be called such as a functor
			auto *fn = dynamic_cast<FnPrototypeStmt*>(symbol);

			if (fn != nullptr)
			{
				expr->type = fn->return_type;
				expr->prototype = fn;
			}
			else
			{
				auto *type = new_node<PendingType>();
				type->symbol = identifier;
				expr->type = type;

				add_symbol_resolved_task(expr->symbol, expr, &Parser::patch_call_expr_type);
			}

			expr->arguments = collect_call_arguments();

			m_lexer.expect(RightParen);

			return expr;
		}

		auto *expr = new_node<SymbolExpr>();

		expr->symbol = identifier;

		auto *var = dynamic_cast<VarDeclStmt*>(symbol);

		if (var != nullptr)
		{
			expr->type = var->type;
			expr->symbol_node = var;

			if (expr->type == nullptr)
			{
				m_post_resolved_tasks.emplace_back(
				UnresolvedSymbol
				{
					.node = expr,
					.task = &Parser::patch_identifier_type
				});
			}
		}

		return expr;


	}
	if (m_lexer.match(Sizeof))
	{
		auto *expr = new_node<SizeofExpr>();

		expr->type = const_cast<Integer*>(&I32Type);

		expr->expr = expression();

		return expr;
	}

	auto *literal = new_node<LiteralExpr>();

	if (m_lexer.match(True) || m_lexer.match(False))
	{
		literal->value = m_lexer.peek_last(True);
		literal->type = const_cast<Bool*>(&BoolType);
	}
	if (m_lexer.match(IntegerLiteral))
	{
		auto lexeme = m_lexer.peek_last().lexeme;
		i32 n = 0;
		std::from_chars(lexeme.begin(), lexeme.end(), n);
		literal->type = const_cast<Integer*>(&I32Type);
		literal->value = n;
	}
	if (m_lexer.match(DecimalLiteral))
	{
		auto lexeme = m_lexer.peek_last().lexeme;
		f32 n = 0;
		std::from_chars(lexeme.begin(), lexeme.end(), n);
		literal->type = const_cast<Float*>(&F32Type);
		literal->value = n;
	}
	if (m_lexer.match(StringLiteral))
	{
		literal->value = m_lexer.peek_last().lexeme;
		literal->type = const_cast<Str*>(&StrType);
	}
	if (m_lexer.match(CharLiteral))
	{
		literal->value = m_lexer.peek_last().lexeme[0];
		literal->type = const_cast<Char*>(&CharType);
	}

	return literal;
}

void pars::Parser::add_symbol_resolved_task(std::string_view name, Node *node, SymbolTask task)
{
	m_pending_symbols[name].emplace_back
	(
		UnresolvedSymbol
		{
			.node = node,
			.task = task
		}
	);
}

void pars::Parser::patch_call_expr_type(Node *node)
{
	auto *expr = dynamic_cast<CallExpr*>(node);

	if (expr != nullptr)
	{
		auto *symbol = m_scope_table.find_symbol(expr->symbol);
		expr->prototype = dynamic_cast<FnPrototypeStmt*>(symbol);

		auto *pending = dynamic_cast<PendingType*>(expr->type);

		if (expr->prototype != nullptr)
		{
			expr->type = expr->prototype->return_type;
		}

		pending->resolved_type = expr->type;
	}
}

void pars::Parser::patch_var_init_type(Node *node)
{
	auto *var = dynamic_cast<VarDeclStmt*>(node);

	if (var != nullptr)
	{
		auto explicit_type = var->type;

		var->type = var->initializer->type;

		if (explicit_type != nullptr && !explicit_type->is_equal(var->type))
		{
			throw FrontendError
			{
				var->token,
				fmt::format
				(
					"cannot initialize variable {} of type {} with type {}",
					var->symbol.name, explicit_type->get_type_name(), var->initializer->type->get_type_name()
				)
			};
		}
	}
}

void pars::Parser::patch_identifier_type(Node *node)
{
	auto *expr = dynamic_cast<SymbolExpr*>(node);

	if (expr != nullptr)
	{
		expr->type = dynamic_cast<VarDeclStmt*>(expr->symbol_node)->type;
	}
}

pars::FnPrototypeStmt * pars::Parser::get_current_fn()
{
	if (m_function_stack.empty())
	{
		return nullptr;
	}

	return m_function_stack.back();
}

bool pars::Parser::followed_by_body()
{
	return m_lexer.peek(LeftBrace) || m_lexer.peek(Arrow);
}

pars::Type* pars::Parser::resolve_type()
{
	auto type_name = m_lexer.expect(Identifier).lexeme;

	auto *type = m_scope_table.find_symbol<Type>(type_name);

	if (type == nullptr)
	{
		throw FrontendError{m_lexer.peek_last(), "unknown type", nullptr};
	}

	return type;
}

std::vector<pars::Expr*> pars::Parser::collect_call_arguments()
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

pars::Expr* pars::Parser::parse_unary()
{
	if (m_lexer.match(Bang) || m_lexer.match(Minus))
	{
		auto op = m_lexer.peek_last();

		auto expr = parse_unary();

		auto *unary = new_node<UnaryExpr>();

		unary->op = op.lexeme[0];
		unary->right = expr;

		expr->type = expr->type;

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


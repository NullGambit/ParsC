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
	struct ParseCtx;
	struct GlobalSymbol;

	// a per module abstract syntax tree
	class AST
	{
	public:

		AST();

		const std::vector<Node*>& parse(ParseCtx *ctx);

		ParseCtx* get_ctx() const;

		u32 get_file_id() const;

	private:
		typedef Expr*(AST::*BinaryRule)();

		ParseCtx *m_ctx;

		Lexer m_lexer {};
		std::vector<Node*> m_nodes;

		std::vector<Node*> *m_target;

		std::vector<FnDecl*> m_function_stack;

		std::vector<Expr*> m_pending_attributes;

		HashMap<std::string_view, std::function<Expr*(const std::vector<Expr*>&)>> m_builtin_functions;

		Node* declaration();
		Node* statement();
		void declare_to(std::vector<Node*> &nodes);
		Node* parse_import();
		VarDeclStmt* parse_var();
		Node* parse_return();
		void parse_attributes();
		Symbol get_symbol();
		FnSignature parse_fn_signature();
		FnDecl* parse_fn(FnFlags flags);
		AliasType* parse_alias();

		// expr parsing stuff
		// TODO replace with a pratt parser
		Expr* expression();
		Expr* parse_equality();
		Expr* parse_comparison();
		Expr* parse_term();
		Expr* parse_unary();
		Expr* parse_factor();
		Expr* parse_primary();

		FnDecl* get_current_fn();

		bool followed_by_body();

		template<IsNode T>
		T* new_node()
		{
			auto *node = pars::new_node<T>();

			node->token = m_lexer.peek_last();

			return node;
		}

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

				auto *right = std::invoke(rule, this);

				auto *bin = new_node<BinaryExpr>();

				bin->left = expr;
				bin->right = right;
				bin->op = op[0];

				expr = bin;

				expr->type = bin->left->type;
			}

			return expr;
		}
	};
}

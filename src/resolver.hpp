#pragma once
#include <string_view>

#include "visitor.hpp"

namespace pars
{
	struct Token;
	class ScopeTable;
	struct ParseCtx;
	struct Type;

	Type* resolve_type(ScopeTable &scope_table, std::string_view symbol, Token &error_token);

	class Resolver : public Visitor
	{
	public:
		explicit Resolver(ParseCtx *ctx) :
			m_ctx{ctx}
		{}

		void visit(FnDecl *fn) override;
		void resolve(const std::vector<Node*> &nodes);

	private:
		ParseCtx *m_ctx;
	};
}

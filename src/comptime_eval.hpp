#pragma once

#include <optional>

#include "analyzer.hpp"
#include "visitor.hpp"

namespace pars
{
	struct ParseCtx;

	struct ComptimeEval : Visitor
	{
		void visit(VarDeclStmt *stmt, VisitCtx ctx) override;
		void visit(SymbolExpr *expr, VisitCtx ctx) override;
		void visit(LiteralExpr *expr, VisitCtx ctx) override;
		void visit(CallExpr *expr, VisitCtx ctx) override;
		void visit(BinaryExpr *expr, VisitCtx ctx) override;

		ComptimeEval() = default;

		explicit ComptimeEval(ParseCtx * ctx);

	private:
		ParseCtx *m_ctx;
	};
}

#pragma once
#include "frontend/visitor.hpp"

namespace pars
{
	struct AstPrinter : Visitor
	{
		void visit(ImportStmt *stmt) override;
		void visit(VarStmt *stmt) override;
		void visit(FnStmt *stmt) override;
		void visit(LiteralExpr *expr) override;
		void visit(BinaryExpr *expr) override;
		void visit(ReturnStmt *stmt) override;
		void visit(SymbolExpr *expr) override;
	};
}

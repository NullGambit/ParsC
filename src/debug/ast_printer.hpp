#pragma once
#include "../visitor.hpp"

namespace pars
{
	struct AstPrinter : Visitor
	{
		void visit(ImportStmt *stmt) override;
		void visit(VarDeclStmt *stmt) override;
		void visit(FnPrototypeStmt *stmt) override;
		void visit(BlockStmt *stmt) override;
		void visit(LiteralExpr *expr) override;
		void visit(BinaryExpr *expr) override;
		void visit(ReturnStmt *stmt) override;
		void visit(SymbolExpr *expr) override;
		void visit(CallExpr *expr) override;
		void visit(PrintlnStmt *stmt) override;
		void visit(ExprFnStmt *stmt) override;
	};
}

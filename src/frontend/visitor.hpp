#pragma once

namespace pars
{
	struct FnStmt;
	struct VarStmt;
	struct ImportStmt;
	struct SymbolExpr;
	struct LiteralExpr;
	struct FnCallExpr;
	struct UnaryExpr;
	struct BinaryExpr;

	struct Visitor
	{
		virtual ~Visitor() = default;

		virtual void visit(BinaryExpr *expr) {}
		virtual void visit(UnaryExpr *expr) {}
		virtual void visit(FnCallExpr *expr) {}
		virtual void visit(LiteralExpr *expr) {}
		virtual void visit(SymbolExpr *expr) {}
		virtual void visit(ImportStmt *stmt) {}
		virtual void visit(VarStmt *stmt) {}
		virtual void visit(FnStmt *stmt) {}
	};
}
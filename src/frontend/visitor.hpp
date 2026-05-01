#pragma once

namespace pars
{
	struct PrintlnStmt;
	struct GroupExpr;
	struct ReturnStmt;
	struct FnStmt;
	struct VarStmt;
	struct ImportStmt;
	struct SymbolExpr;
	struct LiteralExpr;
	struct CallExpr;
	struct UnaryExpr;
	struct BinaryExpr;

	struct Visitor
	{
		virtual ~Visitor() = default;

		virtual void visit(BinaryExpr *expr) {}
		virtual void visit(UnaryExpr *expr) {}
		virtual void visit(CallExpr *expr) {}
		virtual void visit(LiteralExpr *expr) {}
		virtual void visit(SymbolExpr *expr) {}
		virtual void visit(GroupExpr *expr) {}
		virtual void visit(ImportStmt *stmt) {}
		virtual void visit(VarStmt *stmt) {}
		virtual void visit(FnStmt *stmt) {}
		virtual void visit(ReturnStmt *stmt) {}
		virtual void visit(PrintlnStmt *stmt) {}
	};
}
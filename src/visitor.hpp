#pragma once

namespace pars
{
	struct AliasType;
	struct FnDecl;
	struct Symbol;
	struct GroupExpr;
	struct ReturnStmt;
	struct VarDeclStmt;
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
		virtual void visit(VarDeclStmt *stmt) {}
		virtual void visit(ReturnStmt *stmt) {}
		virtual void visit(Symbol *stmt) {}
		virtual void visit(FnDecl *stmt) {}
		virtual void visit(AliasType *stmt) {}
	};
}
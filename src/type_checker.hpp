#pragma once
#include "visitor.hpp"

namespace pars
{
	class AST;

	struct TypeChecker : Visitor
	{
		AST &ast;

		explicit TypeChecker(AST &ast) :
			ast{ast}
		{
		}

		void visit(CallExpr *expr) override;
		void visit(FnDecl *stmt) override;
		void visit(VarDeclStmt *stmt) override;
	};
}

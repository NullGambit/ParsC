#pragma once
#include "frontend/visitor.hpp"

namespace pars
{
	struct AstPrinter : Visitor
	{
		void visit(ImportStmt *stmt) override;
		void visit(VarStmt *stmt) override;
		void visit(FnStmt *stmt) override;
	};
}

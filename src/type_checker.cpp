#include "type_checker.hpp"

#include "ast.hpp"

void pars::TypeChecker::visit(CallExpr *expr)
{
	if (expr->prototype == nullptr)
	{
		expr->prototype = dynamic_cast<FnDecl*>(ast.get_scope_table().find_symbol(expr->symbol));
		expr->type = expr->prototype->signature.return_type;
	}
}

void pars::TypeChecker::visit(FnDecl *stmt)
{
	for (auto *node : stmt->body)
	{
		node->accept(this);
	}
}

void pars::TypeChecker::visit(VarDeclStmt *stmt)
{
	if (stmt->type == nullptr && stmt->initializer != nullptr)
	{
		stmt->initializer->accept(this);
		stmt->type = stmt->initializer->type;
	}
}

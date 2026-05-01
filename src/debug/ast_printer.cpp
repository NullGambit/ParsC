#include "ast_printer.hpp"

#include "frontend/expr.hpp"
#include "frontend/stmt.hpp"
#include "util/fmt.hpp"

void pars::AstPrinter::visit(ImportStmt *stmt)
{
	fmt::println("import {}", stmt->path);
}

void pars::AstPrinter::visit(VarStmt *stmt)
{
	fmt::print("var {}: type<{}>", stmt->symbol, stmt->type);

	if (stmt->initializer != nullptr)
	{
		fmt::print(" init<");

		stmt->initializer->accept(this);

		fmt::print(">");
	}

	fmt::println("");
}

void pars::AstPrinter::visit(FnStmt *stmt)
{
	fmt::println("fn: {}", stmt->symbol);

	fmt::print("params: (");

	for (auto i = 0; auto [name, type] : stmt->prototype.parameters)
	{
		fmt::print("{}: type<{}>", name, type);

		if (i++ < stmt->prototype.parameters.size() - 1)
		{
			fmt::print(", ");
		}
	}

	fmt::println("): {}", stmt->prototype.return_type);

	fmt::println("Body: ");

	for (auto body_stmt : stmt->body)
	{
		body_stmt->accept(this);
	}
}

void pars::AstPrinter::visit(LiteralExpr *expr)
{
	fmt::print("{}", std::get<i64>(expr->value));
}

void pars::AstPrinter::visit(BinaryExpr *expr)
{
	expr->left->accept(this);
	fmt::print(" {} ", expr->op);
	expr->right->accept(this);
}

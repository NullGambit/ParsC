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
	fmt::println("var {}: type<{}>", stmt->symbol, stmt->type);
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

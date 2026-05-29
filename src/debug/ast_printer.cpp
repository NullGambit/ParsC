// #include "ast_printer.hpp"
//
// #include "../expr.hpp"
// #include "../stmt.hpp"
// #include "magic_enum/magic_enum.hpp"
// #include "util/fmt.hpp"
//
// void pars::AstPrinter::visit(ImportStmt *stmt)
// {
// 	// fmt::println("import {}", stmt->path);
// }
//
// void pars::AstPrinter::visit(VarDeclStmt *stmt)
// {
// 	fmt::print("var {}: type<{}>", stmt->symbol.name, stmt->type);
//
// 	if (stmt->initializer != nullptr)
// 	{
// 		fmt::print(" init<");
//
// 		stmt->initializer->accept(this);
//
// 		fmt::print(">");
// 	}
//
// 	fmt::println("");
// }
//
// void pars::AstPrinter::visit(FnDecl *stmt)
// {
// 	fmt::println("fn : {}", stmt->symbol.name);
//
// 	if (stmt->symbol.attribute_id != NO_ATTRIBUTES)
// 	{
// 		fmt::print("Attributes: ");
//
// 		auto attributes = get_attributes(stmt->symbol);
//
// 		for (auto i = 0; auto *attr : attributes)
// 		{
// 			attr->accept(this);
//
// 			if (i++ < attributes.size() - 1)
// 			{
// 				fmt::print(", ");
// 			}
// 		}
//
// 		fmt::println("");
// 	}
// 	fmt::print("params: (");
//
// 	for (auto i = 0; auto var : stmt->signature.parameters)
// 	{
// 		fmt::print("{}: type<{}>", var->symbol.name, var->type);
//
// 		if (i++ < stmt->signature.parameters.size() - 1)
// 		{
// 			fmt::print(", ");
// 		}
// 	}
//
// 	fmt::println("): {}", stmt->signature.return_type);
//
// 	for (auto *entry : stmt->body)
// 	{
// 		entry->accept(this);
// 	}
// }
//
//
// void pars::AstPrinter::visit(LiteralExpr *expr)
// {
// 	// fmt::print("{}", std::get<i32>(expr->value));
// }
//
// void pars::AstPrinter::visit(BinaryExpr *expr)
// {
// 	expr->left->accept(this);
// 	fmt::print(" {} ", magic_enum::enum_name(expr->op));
// 	expr->right->accept(this);
// }
//
// void pars::AstPrinter::visit(ReturnStmt *stmt)
// {
// 	fmt::print("Return <");
// 	stmt->expr->accept(this);
// 	fmt::println(">");
// }
//
// void pars::AstPrinter::visit(SymbolExpr *expr)
// {
// 	fmt::print("{}", expr->symbol);
// }
//
// void pars::AstPrinter::visit(CallExpr *expr)
// {
// 	fmt::print("call {}", expr->symbol);
//
// 	if (!expr->arguments.empty())
// 	{
// 		fmt::print("(");
// 	}
//
// 	for (auto i = 0; auto *arg : expr->arguments)
// 	{
// 		arg->accept(this);
//
// 		if (i++ < expr->arguments.size() - 1)
// 		{
// 			fmt::print(", ");
// 		}
// 	}
//
// 	if (!expr->arguments.empty())
// 	{
// 		fmt::print(")");
// 	}
// }
//
// void pars::AstPrinter::print(std::span<Node *> nodes)
// {
// 	for (auto *node : nodes)
// 	{
// 		node->accept(this);
// 	}
// }
//

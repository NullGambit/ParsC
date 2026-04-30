#include <iostream>
#include <vector>

#include "overload.hpp"
#include "debug/token_printer.hpp"
#include "frontend/file_manager.hpp"
#include "frontend/lexer.hpp"
#include "frontend/parser.hpp"
#include "frontend/token.hpp"
#include "util/fmt.hpp"
#include "util/io.hpp"

int main()
{
    // auto source = pars::read_file("./tests/tokens.pars");
    //
    // if (!source.has_value())
    // {
    //     fmt::panic("could not read source file");
    // }
    //
    // pars::print_tokens(source.value());

    auto source = pars::load_file("./tests/tokens.pars");

    if (!source.has_value())
    {
	    fmt::panic("could not read source file");
    }

	pars::print_tokens(source.value());

	// auto parser = pars::Parser();
	//
	// auto statements = parser.parse(source.value());
	//
	// for (auto &stmt_id : statements)
	// {
	// 	auto &stmt = pars::get_stmt(stmt_id);
	//
	// 	std::visit(ccc::overload
	// 	{
	// 		[](pars::ImportStmt &stmt)
	// 		{
	// 			for (auto path : stmt.path)
	// 			{
	// 				fmt::println(path);
	// 			}
	// 		},
	// 		[](pars::TypedSymbol &stmt)
	// 		{
	//
	// 		},
	// 		[](pars::FnStmt &stmt)
	// 		{
	//
	// 		},
	// 		[](pars::VarDeclStmt &stmt)
	// 		{
	//
	// 		}
	// 	}, stmt);
	// }
}

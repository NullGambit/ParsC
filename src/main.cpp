#include <iostream>
#include <vector>

#include "overload.hpp"
#include "debug/ast_printer.hpp"
#include "debug/token_printer.hpp"
#include "frontend/file_manager.hpp"
#include "frontend/lexer.hpp"
#include "frontend/parser.hpp"
#include "frontend/token.hpp"
#include "util/fmt.hpp"

int main()
{
    auto source = pars::load_file("./tests/parsing.pars");

    if (!source.has_value())
    {
	    fmt::panic("could not read source file");
    }
 
	// pars::print_tokens(source.value());

	auto parser = pars::Parser();

	try
	{
		auto statements = parser.parse(source.value());
		auto ast_printer = pars::AstPrinter{};

		for (auto *stmt : statements)
		{
			if (stmt != nullptr)
			{
				stmt->accept(&ast_printer);
			}
		}

		pars::free_memory_blocks();
	}
	catch (std::exception &e)
	{
		fmt::println("{}", e.what());
	}
}

#include <iostream>
#include <vector>

#include "overload.hpp"
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

	pars::print_tokens(source.value());

	auto stmt = pars::new_node<pars::VarStmt>();

	stmt->symbol = "hello";

	fmt::println("{}", stmt->symbol.size());

	auto parser = pars::Parser();

	auto statements = parser.parse(source.value());

	pars::free_memory_blocks();
}

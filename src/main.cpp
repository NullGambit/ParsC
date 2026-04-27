#include <iostream>
#include <vector>

#include "debug/token_printer.hpp"
#include "frontend/lexer.hpp"
#include "frontend/token.hpp"
#include "util/fmt.hpp"
#include "util/io.hpp"

int main()
{
    auto source = pars::read_file("./tests/tokens.pars");

    if (!source.has_value())
    {
        fmt::fatal("could not read source file");
    }

    pars::print_tokens(source.value());
}

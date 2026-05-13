#include <iostream>
#include <vector>

#include "compile_error.hpp"
#include "compiler.hpp"
#include "overload.hpp"
#include "debug/ast_printer.hpp"
#include "debug/token_printer.hpp"
#include "file_manager.hpp"
#include "lexer.hpp"
#include "ast.hpp"
#include "module.hpp"
#include "module_manager.hpp"
#include "token.hpp"
#include "type.hpp"
#include "util/fmt.hpp"

int main()
{
	constexpr auto SOURCE_PATH = "./tests/compile.pars";

	try
	{
		auto *main_module = pars::get_module(SOURCE_PATH);

		if (main_module == nullptr)
		{
			fmt::panic("Could not read main module");
		}

		main_module->module->print(llvm::outs(), nullptr);

		auto ctx = main_module->make_ctx();

		pars::compile_exe(ctx, "./a");

		pars::free_memory_blocks();
	}
	catch (std::exception &e)
	{
		fmt::println("\n{}", e.what());
	}
}

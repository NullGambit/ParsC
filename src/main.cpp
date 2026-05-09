#include <iostream>
#include <vector>

#include "compile_error.hpp"
#include "compiler.hpp"
#include "overload.hpp"
#include "debug/ast_printer.hpp"
#include "debug/token_printer.hpp"
#include "file_manager.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "token.hpp"
#include "type.hpp"
#include "util/fmt.hpp"

int main()
{
    auto source = pars::load_file("./tests/compile.pars");

    if (!source.has_value())
    {
	    fmt::panic("could not read source file");
    }

	// pars::print_tokens(source.value());

	auto parser = pars::Parser();

	try
	{
		auto &statements = parser.parse(source.value());

		parser.resolve_symbols();

		auto ast_printer = pars::AstPrinter{};

		for (auto *stmt : statements)
		{
			if (stmt != nullptr)
			{
				stmt->accept(&ast_printer);
			}
		}

		std::flush(std::cout);

		auto compiler = pars::Compiler{statements, pars::EmitCtx{}};

		compiler.ctx.llvm_ctx = std::make_unique<llvm::LLVMContext>();
		compiler.ctx.module = std::make_unique<llvm::Module>("main", *compiler.ctx.llvm_ctx);
		compiler.ctx.builder = std::make_unique<llvm::IRBuilder<>>(*compiler.ctx.llvm_ctx);

		for (auto *stmt : statements)
		{
			stmt->accept(&compiler);
		}

		compiler.ctx.module->print(llvm::outs(), nullptr);

		pars::compile_exe(compiler.ctx, "./a");

		pars::free_memory_blocks();
	}
	catch (std::exception &e)
	{
		fmt::println("\n{}", e.what());
	}
}

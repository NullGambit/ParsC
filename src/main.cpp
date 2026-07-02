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

void init_global_symbols();

int main(int argc, char **argv)
{
	// TODO: if entry file is not provided parse and compile all files in dir
	if (argc < 2)
	{
		fmt::panic("Must provide an entry file\n");
	}

	// TODO make this cross-platform
	auto exe_path = std::filesystem::canonical("/proc/self/exe");

	// remove the executables name
	exe_path.remove_filename();

	auto source_path = std::filesystem::path{argv[1]};

	pars::add_module_path("./");
	pars::add_module_path(exe_path);

	try
	{
		init_global_symbols();

		auto *main_module = pars::get_module(source_path);

		if (main_module == nullptr)
		{
			fmt::panic("Could not read main module");
		}

		pars::compile_exe("./a");
	}
	catch (std::exception &e)
	{
		fmt::println("\n{}", e.what());
	}
}

void init_global_symbols()
{
	auto declare_global_type = [&](const pars::Type *type)
	{
		pars::declare_global_symbol(type->get_type_name(),
		{
			.node = const_cast<pars::Type*>(type),
			.availability = pars::GlobalSymbolAvailability::Always
		});
	};

	declare_global_type(&pars::VoidType);
	declare_global_type(&pars::I8Type);
	declare_global_type(&pars::U8Type);
	declare_global_type(&pars::I16Type);
	declare_global_type(&pars::U16Type);
	declare_global_type(&pars::I32Type);
	declare_global_type(&pars::U32Type);
	declare_global_type(&pars::I64Type);
	declare_global_type(&pars::U64Type);
	declare_global_type(&pars::BoolType);
	declare_global_type(&pars::StrType);
	declare_global_type(&pars::CharType);
	declare_global_type(&pars::UCharType);
	declare_global_type(&pars::F32Type);
	declare_global_type(&pars::F64Type);
}

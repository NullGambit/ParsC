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
#include "cli.hpp"
#include "config.hpp"
#include "module.hpp"
#include "module_manager.hpp"
#include "path.hpp"
#include "token.hpp"
#include "type.hpp"
#include "warnings.hpp"
#include "magic_enum/magic_enum.hpp"
#include "util/fmt.hpp"

void init_global_symbols();

struct Wrapper
{
	pars::Type *type;

	pars::Type* operator->() const
	{
		return type;
	}
};

int main(int argc, char **argv)
{
	// TODO: if entry file is not provided parse and compile all files in dir
	if (argc < 2)
	{
		fmt::panic("Must provide an entry file\n");
	}

	auto &config = pars::init_config();

	auto result = pars::parse_cli_args(argc, argv,
	{
		.needs_command = true,
	});

	if (result.has_error())
	{
		fmt::panic("{}", result.message);
	}

	if (config.command == pars::CompileCommand::Help)
	{
		pars::print_cli_help();
		return 0;
	}

	auto exe_path = pars::get_exe_path();

	// remove the executables name
	exe_path.remove_filename();

	auto source_path = std::filesystem::path{result.args[0]};

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

		pars::display_warnings();

		auto filename = source_path.filename().replace_extension("");

		auto out = config.out.empty() ? filename.c_str() : config.out;

		if (config.should_compile())
		{
			pars::compile_exe(out);
		}

		if (config.command == pars::CompileCommand::Run)
		{
			std::string args;

			for (auto iter = result.args.begin() + 1; iter != result.args.end(); ++iter)
			{
				args += *iter;
				args += " ";
			}

			fmt::println("{}", args);

			system(fmt::format("{} {}", out, args).data());
		}
	}
	catch (std::exception &e)
	{
		if (!config.silent_errors)
		{
			fmt::println("\n{}", e.what());
		}

		return -1;
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

	declare_global_type(&pars::VOID_TYPE);
	declare_global_type(&pars::I8_TYPE);
	declare_global_type(&pars::U8_TYPE);
	declare_global_type(&pars::I16_TYPE);
	declare_global_type(&pars::U16_TYPE);
	declare_global_type(&pars::I32_TYPE);
	declare_global_type(&pars::U32_TYPE);
	declare_global_type(&pars::I64_TYPE);
	declare_global_type(&pars::U64_TYPE);
	declare_global_type(&pars::BOOL_TYPE);
	declare_global_type(&pars::StrType);
	declare_global_type(&pars::CHAR_TYPE);
	declare_global_type(&pars::UCHAR_TYPE);
	declare_global_type(&pars::F32_TYPE);
	declare_global_type(&pars::F64_TYPE);
}

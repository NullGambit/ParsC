#pragma once
#include <string_view>

namespace pars
{
	enum class CompileCommand : u32
	{
		Build,
		Run,
		Help,
		Test,
	};

	struct Config
	{
		CompileCommand command;
		bool emit_llvm;
		bool do_not_compile;
		bool do_not_warn;
		bool fail_on_warning;
		bool silent_errors;
		std::string_view out;
		i32 opt_level {};

		bool should_compile() const;
	};

	const Config& init_config();

	const Config& get_config();
}

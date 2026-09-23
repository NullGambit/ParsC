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
		std::string_view out;
	};

	void init_config();

	const Config& get_config();
}

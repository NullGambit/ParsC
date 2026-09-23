#include "config.hpp"

#include "cli.hpp"

static pars::Config g_config {};

void pars::init_config()
{
	set_cli_command("build", CompileCommand::Build, &g_config.command);
	set_cli_command("run", CompileCommand::Run, &g_config.command);
	set_cli_command("test", CompileCommand::Test, &g_config.command);

	add_cli_switch
	({
		.name = "emit-llvm",
		.alias = "ellvm",
		.buffer = &g_config.emit_llvm
	});

	add_cli_switch
	({
		.name = "do-not-compile",
		.alias = "nocomp",
		.buffer = &g_config.do_not_compile
	});

	add_cli_switch
	({
		.name = "out",
		.alias = "o",
		.buffer = &g_config.out
	});
}

const pars::Config & pars::get_config()
{
	return g_config;
}

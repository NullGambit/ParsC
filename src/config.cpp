#include "config.hpp"

#include "cli.hpp"

static pars::Config g_config {};

bool pars::Config::should_compile() const
{
	return !do_not_compile && command == CompileCommand::Build || command == CompileCommand::Run;
}

void pars::init_config()
{
#define CMD(C) .index = (u32)C, .buffer = (u32*)&g_config.command

	set_cli_command("build",
	{
		CMD(CompileCommand::Build),
		.description = "builds the file into an executable"
	});
	set_cli_command("run",
	{
		CMD(CompileCommand::Run),
		.description = "builds the file into an executable and then runs it directly"
	});
	set_cli_command("help",
	{
		CMD(CompileCommand::Help),
		.description = "shows the help command"
	});
	set_cli_command("test",
	{
		CMD(CompileCommand::Test),
		.description = "runs all unit tests. (currently not implemented)"
	});

#undef CMD

	add_cli_switch
	({
		.name = "emit-llvm",
		.alias = "llvm",
		.description = "emits llvm ir",
		.buffer = &g_config.emit_llvm
	});

	add_cli_switch
	({
		.name = "do-not-compile",
		.alias = "nocomp",
		.description = "will not compile. useful when combined with other switches.",
		.buffer = &g_config.do_not_compile
	});

	add_cli_switch
	({
		.name = "out",
		.alias = "o",
		.description = "sets the output path",
		.buffer = &g_config.out
	});

	add_cli_switch
	({
		.name = "no-warnings",
		.alias = "nowarn",
		.description = "will supress warnings",
		.buffer = &g_config.do_not_warn
	});

	add_cli_switch
	({
		.name = "fail-on-warning",
		.alias = "fow",
		.description = "will stop compilation on any error",
		.buffer = &g_config.fail_on_warning
	});
}

const pars::Config & pars::get_config()
{
	return g_config;
}

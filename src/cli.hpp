#pragma once
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace pars
{
	using CliSwitchValue = std::variant<bool*, i32*, std::string_view*>;

	struct CliSwitch
	{
		std::string_view name;
		std::string_view alias;
		std::string_view description;
		CliSwitchValue buffer;
	};

	struct CliParseOptions
	{
		bool needs_command;
	};

	struct CliParseResult
	{
		std::string message;
		std::vector<std::string_view> args;
	};

	struct CliCommandInfo
	{
		u32 index;
		u32 *buffer;
		std::string_view description;
	};

	void set_cli_command(std::string_view name, CliCommandInfo info);

	void add_cli_switch(CliSwitch cli_switch);

	// parses cli arguments respective to the switches and commands that have been registered
	// returns a message if an error was found but otherwise will be empty
	CliParseResult parse_cli_args(i32 argc, char **argv, CliParseOptions options);

	void print_cli_help();
}

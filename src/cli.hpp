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

	void set_cli_command_raw(std::string_view name, u32 index, u32 *buffer);

	template<class T>
	void set_cli_command(std::string_view name, T index, T *buffer)
	{
		set_cli_command_raw(name, (u32)index, (u32*)buffer);
	}

	void add_cli_switch(CliSwitch cli_switch);

	// parses cli arguments respective to the switches and commands that have been registered
	// returns a message if an error was found but otherwise will be empty
	CliParseResult parse_cli_args(i32 argc, char **argv, CliParseOptions options);
}

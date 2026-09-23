#include "cli.hpp"

#include "overload.hpp"
#include "containers/hash_map.hpp"
#include "util/fmt.hpp"

struct CliCommandInfo
{
	u32 index;
	u32 *buffer;
};

using SwitchMap = pars::HashMap<std::string_view, pars::CliSwitchValue>;

static pars::HashMap<std::string_view, CliCommandInfo> g_cli_commands;
static SwitchMap g_switches;
static SwitchMap g_switches_aliases;

void pars::set_cli_command_raw(std::string_view name, u32 index, u32 *buffer)
{
	g_cli_commands[name] = {index, buffer};
}

void pars::add_cli_switch(CliSwitch cli_switch)
{
	g_switches[cli_switch.name] = cli_switch.buffer;

	if (!cli_switch.alias.empty())
	{
		g_switches_aliases[cli_switch.alias] = cli_switch.buffer;
	}
}

pars::CliParseResult pars::parse_cli_args(i32 argc, char **argv, CliParseOptions options)
{
	auto found_command = false;

	std::vector<std::string_view> args;

	for (auto i = 1; i < argc; i++)
	{
		auto arg = std::string_view{argv[i]};

		SwitchMap *switch_map {};
		auto index = 0;

		if (arg.starts_with("--"))
		{
			switch_map = &g_switches;
			index = 2;
		}
		else if (arg.starts_with("-"))
		{
			switch_map = &g_switches_aliases;
			index = 1;
		}

		if (switch_map != nullptr)
		{
			auto switch_name = arg.substr(index);

			auto iter = switch_map->find(switch_name);

			if (iter == switch_map->end())
			{
				return {fmt::format("no switch by the name of '{}' exists", switch_name)};
			}

			std::visit(overload
			{
				[&](bool *buff)
				{
					*buff = true;
				},
				[&](i32 *buff)
				{
					arg = argv[++i];
					*buff = std::atoi(arg.data());
				},
				[&](std::string_view *buff)
				{
					arg = argv[++i];
					*buff = arg;
				},
			}, iter->second);
		}
		else
		{
			auto iter = g_cli_commands.find(arg);

			if (iter != g_cli_commands.end())
			{
				if (found_command)
				{
					return {"command has already been used"};
				}

				found_command = true;

				*iter->second.buffer = iter->second.index;
			}
			else
			{
				args.emplace_back(arg);
			}
		}
	}

	if (!found_command && options.needs_command)
	{
		return {"expected a command but got none"};
	}

	return {.args = args};
}

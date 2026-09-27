#include <iostream>
#include <c++/12/filesystem>

#include "cli.hpp"
#include "util/fmt.hpp"

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"

int main(int argc, char **argv)
{
	auto result = pars::parse_cli_args(argc, argv, {.needs_command = false});

	if (!result.message.empty())
	{
		fmt::panic(result.message);
	}

	if (result.args.empty())
	{
		fmt::panic("Expected root directory for language tests");
	}

	auto root_path = result.args.front();

	auto passed = 0;
	auto failed = 0;

	for (const auto &entry : std::filesystem::directory_iterator{root_path})
	{
		if (entry.is_directory())
		{
			continue;
		}

		auto result = system(fmt::format("parsc build {} -nowarn -silent -o /tmp/pars_lang_test -nocomp", entry.path()).data());

		if (result == 0)
		{
			passed++;
		}
		else
		{
			failed++;
		}

		auto message = result == 0 ? "passed!" : "failed!";
		auto color = result == 0 ? GREEN : RED;

		fmt::println("{} {} {} {}", entry.path(), color, message, RESET);
	}

	fmt::println("{} passed, {} failed", passed, failed);
}

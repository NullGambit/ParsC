#include "warnings.hpp"

#include <vector>
#include <string>

#include "config.hpp"
#include "memory/arena.hpp"
#include "util/fmt.hpp"
#include "token.hpp"

// the allocation patterns used within warnings and calls to warn such as warn(fmt::format())
// are less than ideal but how many warnings would even a large project really have?
// until it is a problem it might not be worth it optimize
static std::vector<std::string> g_warnings;

void pars::warn(std::string_view message, std::optional<Token> token)
{
	if (get_config().do_not_warn)
	{
		return;
	}

	std::string formatted_message;

	if (token.has_value())
	{
		formatted_message = report_token(token.value(), message, ReportType::Warning);
	}
	else
	{
		formatted_message = fmt::format("[Warning] {}", message);
	}

	g_warnings.emplace_back(formatted_message);
}

void pars::display_warnings()
{
	for (const auto &warning : g_warnings)
	{
		fmt::println(warning);
	}

	if (!g_warnings.empty() && get_config().fail_on_warning)
	{
		fmt::panic("compilation failed due to fail on warning switch\n");
	}
}

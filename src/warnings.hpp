#pragma once
#include <optional>
#include <string_view>

namespace pars
{
	struct Token;

	void warn(std::string_view message, std::optional<Token> token);

	void display_warnings();
}

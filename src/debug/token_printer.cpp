#include "token_printer.hpp"

#include <vector>

#include "containers/hash_map.hpp"
#include "frontend/lexer.hpp"
#include "frontend/token.hpp"
#include "magic_enum/magic_enum.hpp"
#include "util/fmt.hpp"

struct ColumnInfo
{
	std::string_view value;
	u32 max_length;
};

void print_padding(u32 length, char c = ' ')
{
	for (auto i = 0; i < length; i++)
	{
		fmt::print("{}", c);
	}
}

void pars::print_tokens(std::string_view source)
{
	auto lexer = Lexer(source);

	std::vector<Token> stream;

	while (lexer.has_next())
	{
		stream.emplace_back(lexer.advance());
	}

	auto diff = [](int a, int b)
	{
		if (a >= b)
		{
			return a - b;
		}
		if (a <= b)
		{
			return b - a;
		}
		return 0;
	};

	auto longest = 0;

	for (auto token : stream)
	{
		auto s = magic_enum::enum_name(token.type);

		if (s.length() > longest)
		{
			longest = s.length();
		}
	}

	const std::string_view TYPE_COLUMN = "Type";
	const std::string_view LOCATION_COLUMN = "Location";
	const std::string_view LEXEME_COLUMN = "Lexeme";

	fmt::print(TYPE_COLUMN);

	print_padding(diff(longest,TYPE_COLUMN.length()));

	fmt::print(" | ");

	fmt::print(LOCATION_COLUMN);

	fmt::print(" | ");

	fmt::println(LEXEME_COLUMN);

	print_padding(longest + LOCATION_COLUMN.length() + TYPE_COLUMN.length() + LEXEME_COLUMN.length() + 3, '-');

	fmt::println("");

	for (auto token : stream)
	{
		auto s = magic_enum::enum_name(token.type);

		fmt::print("{}", s);

		print_padding(diff(longest, s.length()));

		fmt::print(" | ");

		auto location = fmt::format("{}:{}", token.line, token.column);

		fmt::print(location);

		print_padding(diff(LOCATION_COLUMN.length(), location.length()));

		fmt::print(" | ");

		fmt::println(token.lexeme);
	}
}

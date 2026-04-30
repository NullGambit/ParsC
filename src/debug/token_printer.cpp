#include "token_printer.hpp"

#include <vector>

#include "frontend/file_manager.hpp"
#include "frontend/lexer.hpp"
#include "frontend/token.hpp"
#include "magic_enum/magic_enum.hpp"
#include "util/fmt.hpp"

void print_padding(u32 length, char c = ' ')
{
	for (auto i = 0; i < length; i++)
	{
		fmt::print("{}", c);
	}
}

template<class Fn>
int longest_token_component(std::vector<pars::Token> &stream, Fn fn)
{
	auto longest = 0;

	for (auto token : stream)
	{
		auto s = fn(token);

		if (s.length() > longest)
		{
			longest = s.length();
		}
	}

	return longest;
}

void pars::print_tokens(SourceFile source)
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

	auto longest_type = longest_token_component(stream, [](Token token) { return magic_enum::enum_name(token.type); });
	auto longest_lexeme = longest_token_component(stream, [](Token token) { return token.lexeme; });

	const std::string_view TYPE_COLUMN = "Type";
	const std::string_view LOCATION_COLUMN = "Location";
	const std::string_view LEXEME_COLUMN = "Lexeme";

	fmt::println("{}", source.path);

	fmt::print("| ");

	fmt::print(TYPE_COLUMN);

	print_padding(diff(longest_type,TYPE_COLUMN.length()));

	fmt::print(" | ");

	fmt::print(LOCATION_COLUMN);

	fmt::print(" | ");

	fmt::print(LEXEME_COLUMN);

	fmt::println(" |");

	auto border_size = longest_type + LOCATION_COLUMN.length() + TYPE_COLUMN.length() + LEXEME_COLUMN.length() + 6;

	print_padding(border_size, '-');

	fmt::println("");

	for (auto token : stream)
	{
		fmt::print("| ");

		auto s = magic_enum::enum_name(token.type);

		fmt::print("{}", s);

		print_padding(diff(longest_type, s.length()));

		fmt::print(" | ");

		auto location = fmt::format("{}:{}", token.location.line, token.location.column);

		fmt::print(location);

		print_padding(diff(LOCATION_COLUMN.length(), location.length()));

		fmt::print(" | ");

		fmt::print(token.lexeme);

		print_padding(diff(longest_lexeme, token.lexeme.length()));

		fmt::println(" |");
	}

	print_padding(border_size, '-');

	fmt::println("");
}

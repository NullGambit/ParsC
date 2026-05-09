#include "token.hpp"

#include <string>

#include "file_manager.hpp"
#include "text_reader.hpp"
#include "util/fmt.hpp"
#include "magic_enum/magic_enum.hpp"

bool pars::is_binary_op(TokenType type)
{
	return type > TokenType::_BinaryStart && type < TokenType::_BinaryEnd;
}

std::string pars::report_token(Token token, std::string_view message)
{
	auto source_file = get_source(token.location.file_id);

	// this function is usually called on compilation stopping bugs so nothing here needs to be fast

	std::string_view line;
	TextReader reader {source_file.contents};

	for (auto i = 1; i < token.location.line - 2 && !reader.at_end(); i++)
	{
		line = reader.get_line();
		reader.skip_insignificant();
	}

	std::string padding;

	for (auto i = 0; i < token.location.column - 2; i++)
	{
		padding += ' ';
	}

	return fmt::format("{} ({}:{}) '{}'\n\t{}\n{}^ Reason: {}",
		source_file.path,
		token.location.line,
		token.location.column,
		token.lexeme,
		line,
		padding,
		message);
}

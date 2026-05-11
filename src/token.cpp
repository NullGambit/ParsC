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

	// this function is usually called on compilation stopping errors so nothing here needs to be fast

	std::string_view line;
	TextReader reader {source_file.contents};

	while (reader.get_current_line() <= token.location.line)
	{
		line = reader.get_line();
		reader.skip_insignificant();
	}

	return fmt::format("{} ({}:{}) '{}'\n\t{}\n\t^ Reason: {}",
		source_file.path,
		token.location.line,
		token.location.column,
		token.lexeme,
		line,
		message);
}

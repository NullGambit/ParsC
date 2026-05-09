#include "token.hpp"

#include <string>

#include "file_manager.hpp"
#include "util/fmt.hpp"
#include "magic_enum/magic_enum.hpp"

bool pars::is_binary_op(TokenType type)
{
	return type > TokenType::_BinaryStart && type < TokenType::_BinaryEnd;
}

std::string pars::report_token(Token token, std::string_view message)
{
	auto source_file = get_source(token.location.file_id);

	return fmt::format("{} ({}:{}) '{}'\n\t{}",
		source_file.path,
		token.location.line,
		token.location.column,
		token.lexeme,
		message);
}

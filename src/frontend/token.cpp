#include "token.hpp"

#include <string>

#include "util/fmt.hpp"
#include "magic_enum/magic_enum.hpp"

std::string pars::to_string(Token token)
{
    return fmt::format("{}({}, {}:{})",
        magic_enum::enum_name(token.type),
        token.lexeme,
        token.line,
        token.column);
}

bool pars::is_binary_op(TokenType type)
{
	return type > TokenType::_BinaryStart && type < TokenType::_BinaryEnd;
}

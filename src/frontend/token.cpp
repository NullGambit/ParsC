#include "token.hpp"

#include <string>

#include "util/fmt.hpp"
#include "magic_enum/magic_enum.hpp"

bool pars::is_binary_op(TokenType type)
{
	return type > TokenType::_BinaryStart && type < TokenType::_BinaryEnd;
}

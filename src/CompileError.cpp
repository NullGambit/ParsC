#include "CompileError.hpp"

const char * pars::CompileError::what() const noexcept
{
	return message.data();
}

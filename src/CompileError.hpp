#pragma once
#include <exception>
#include <string>

namespace llvm
{
	class Value;
}

namespace pars
{
	struct CompileError : std::exception
	{
		std::string message;

		CompileError(std::string &&message) :
		message{message}
		{}

		const char *what() const noexcept override;
	};
}

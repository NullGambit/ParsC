#pragma once
#include <exception>
#include <string>

namespace llvm
{
	class Value;
}

namespace pars
{
	struct Node;

	struct CompileError : std::exception
	{
		Node *node;
		std::string message;

		CompileError(Node *node, std::string &&message);

		const char *what() const noexcept override;
	};
}

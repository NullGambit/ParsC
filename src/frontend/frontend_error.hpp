#pragma once
#include <exception>
#include <string>

#include "token.hpp"

namespace pars
{
	struct Node;
	// i really hate exceptions but
	// for this project i decided to use exceptions to handle errors that are supposed to stop compilation
	// because C++ really has terrible support for errors as values
	// and no support for propagating such errors
	// exceptions solve this problem well enough
	struct FrontendError : std::exception
	{
		Token token;
		Node *node;
		std::string message;

		FrontendError(Token token, std::string &&message, Node *node = nullptr);

		const char *what() const noexcept override;
	};
}

#include "compile_error.hpp"

#include "token.hpp"
#include "node.hpp"

pars::CompileError::CompileError(Node *node, std::string &&message) :
		node{node},
		message{message}
{
	if (node != nullptr)
	{
		this->message = report_token(node->token, message);
	}
}

const char* pars::CompileError::what() const noexcept
{
	return message.data();
}

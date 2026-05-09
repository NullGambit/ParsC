#include "frontend_error.hpp"

#include "file_manager.hpp"
#include "util/fmt.hpp"

pars::FrontendError::FrontendError(Token token, std::string &&message, Node *node)
{
	this->token = token;
	this->node = node;

	if (!message.empty())
	{
		this->message = message;
	}

	this->message = report_token(token, message);
}

const char * pars::FrontendError::what() const noexcept
{
	return message.data();
}

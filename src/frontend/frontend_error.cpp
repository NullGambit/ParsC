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

	auto source_file = get_source(token.location.file_id);

	this->message = fmt::format("{} ({}:{}) '{}'\n\t{}",
		source_file.path,
		token.location.line,
		token.location.column,
		token.lexeme,
		message);
}

const char * pars::FrontendError::what() const noexcept
{
	return message.data();
}

#pragma once

#include "emit_context.hpp"
#include "ast.hpp"
#include "visitor.hpp"

namespace pars
{
	void compile_exe(std::string_view output_path);
}

#pragma once
#include <span>

#include "util/io.hpp"


namespace pars
{
	struct Module;
	class AST;

	Module* get_module(const std::filesystem::path &path);
}

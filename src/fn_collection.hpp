#pragma once

#include <span>
#include <vector>

#include "node.hpp"

namespace pars
{
	class Analyzer;
	struct Expr;

	struct FnCollection : Node
	{
		std::vector<FnType*> functions;

		FnType* get_fn(std::span<Expr*> args, Analyzer *analyzer);
	};
}

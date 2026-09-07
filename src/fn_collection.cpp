#include "fn_collection.hpp"

#include "analyzer.hpp"
#include "expr.hpp"
#include "type.hpp"

pars::FnType * pars::FnCollection::get_fn(std::span<Expr *> args, Analyzer *analyzer)
{
	if (functions.size() == 1)
	{
		return functions.front();
	}

	for (auto *fn : functions)
	{
		if (fn->signature.callable_arity > args.size())
		{
			continue;
		}

		auto all_match = true;
		auto ambiguous_args = 0;

		for (auto i = 0; auto &arg : args)
		{
			if (i >= fn->signature.parameters.size())
			{
				break;
			}

			auto *param = fn->signature.parameters[i];

			if (arg->type == nullptr && arg->is_ctx_sensitive())
			{
				arg = analyzer->visit_expr(nullptr, arg, {.type = param->type});
				ambiguous_args++;
			}

			if (arg->type == nullptr || !arg->type->is_equal(param->type))
			{
				all_match = false;
				break;
			}

			i++;
		}

		if (ambiguous_args == fn->signature.parameters.size())
		{
			return nullptr;
		}

		if (all_match)
		{
			return fn;
		}
	}

	return nullptr;
}

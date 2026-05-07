#pragma once

#include "emit_context.hpp"
#include "parser.hpp"
#include "visitor.hpp"

namespace pars
{
	// compiles a module from a vector of parsed nodes
	struct Compiler : Visitor
	{
		const std::vector<Node*>& statements;
		EmitCtx ctx;

		Compiler(const std::vector<Node*> &statements, EmitCtx &&ctx) :
			statements{statements},
			ctx{std::forward<EmitCtx>(ctx)}
		{}

		void visit(FnPrototypeStmt *fn) override;
		void visit(BlockStmt *fn) override;
	};
}

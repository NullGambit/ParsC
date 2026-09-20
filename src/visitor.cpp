#include "visitor.hpp"

#include "node.hpp"

void pars::Visitor::visit_nodes(const std::vector<Node *> &nodes)
{
	for (auto *node : nodes)
	{
		node->accept(this, {});
	}
}

pars::VisitCtx pars::Visitor::new_ctx(Node *invoker, const VisitCtx &ctx, Type *type_override)
{
	return
	{
		.type = type_override != nullptr ? type_override : ctx.type,
		.invoker = invoker,
		.parse_ctx_override = ctx.parse_ctx_override,
	};
}

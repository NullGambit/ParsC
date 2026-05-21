#include "visitor.hpp"

#include "node.hpp"

void pars::Visitor::visit_nodes(const std::vector<Node *> &nodes)
{
	for (auto *node : nodes)
	{
		node->accept(this);
	}
}

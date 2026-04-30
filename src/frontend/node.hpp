#pragma once

#include <cstddef>
#include <memory>

namespace pars
{
	struct Visitor;

	struct Node
	{
		virtual ~Node() = default;

		virtual void visit(Visitor*) {}
	};

	Node *alloc_node(u32 size);

	void free_memory_blocks();

	template<class T>
	T* new_node()
	{
		auto *node = alloc_node(sizeof(T));
		return new (node) T;
	}
}

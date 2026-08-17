#pragma once

#include <memory>

#include "emit_context.hpp"
#include "token.hpp"
#include "visitor.hpp"

namespace llvm
{
	class Value;
}

namespace pars
{
	struct VisitCtx;
	struct Type;
	struct EmitCtx;
	struct Visitor;

	struct Node
	{
		// the token associated with every node
		// mainly useful for error reporting
		Token token;

		virtual llvm::Value* emit(EmitCtx &ctx, EmitParams params = {}) { return nullptr; }

		virtual ~Node() = default;

		virtual Node* accept(Visitor *visitor, VisitCtx ctx) { return nullptr; }
	};

#define ACCEPT pars::Node* accept(Visitor *visitor, VisitCtx ctx) override { return visitor->visit(this, ctx); }

	u8 *alloc_node(u32 size);

	// slightly pointless because nodes usually live as long as the compiler is running
	// and thus would be freed up by the os after exit
	// but why not might be useful later
	void free_memory_blocks();

	// might seem arbitrary to force only nodes to use this arena
	// but only allocating ast nodes here will mean ast nodes will be tightly packed
	// and no extra nonsense will be in between thus improving data locality
	template<class T>
	concept IsNode = std::is_base_of_v<Node, T>;

	template<IsNode T>
	T* new_node()
	{
		auto *node = alloc_node(sizeof(T));
		return new (node) T;
	}
}

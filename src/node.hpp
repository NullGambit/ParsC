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
	struct Stmt;
	struct Expr;
	struct VisitCtx;
	struct Type;
	struct EmitCtx;
	struct Visitor;

	struct Node
	{
		// the token associated with every node
		// mainly useful for error reporting
		Token token;

		enum class BaseKind
		{
			Node,
			Expr,
			Stmt,
			Type,
		} base_kind;

		Node() :
			base_kind{BaseKind::Node}
		{}

		Node(BaseKind kind) :
			base_kind{kind}
		{}

		virtual llvm::Value* emit(EmitCtx &ctx, EmitParams params = {}) { return nullptr; }

		virtual ~Node() = default;

		virtual Node* accept(Visitor *visitor, VisitCtx ctx) { return nullptr; }
	};

#define CLASSOF(V, T) T##V() : V{Kind::T} {} static bool classof(const V *other) { return other->kind == Kind::T; }

#define ACCEPT pars::Node* accept(Visitor *visitor, VisitCtx ctx) override { return visitor->visit(this, ctx); }

	template<class To, class From>
	To* dyn_cast(From *from)
	{
		if (from != nullptr && To::classof(from))
		{
			return static_cast<To*>(from);
		}

		return nullptr;
	}

	Expr* to_expr(Node *node);
	Stmt* to_stmt(Node *node);
	Type* to_type(Node *node);

	template<class To, class From>
	bool isa(const From *from)
	{
		return To::classof(from);
	}

	template<class To, class From>
	const To* dyn_cast(const From *from)
	{
		if (from != nullptr && To::classof(from))
		{
			return static_cast<const To*>(from);
		}

		return nullptr;
	}

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

	template<IsNode T, class ...Args>
	T* new_node(Args &&...args)
	{
		auto *node = alloc_node(sizeof(T));
		return new (node) T {std::forward<Args>(args)...};
	}
}

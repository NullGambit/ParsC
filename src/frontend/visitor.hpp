#pragma once

namespace pars
{
	struct FnCallExpr;
	struct UnaryExpr;
	struct BinaryExpr;

	struct Visitor
	{
		virtual ~Visitor() = default;

		virtual void visit(BinaryExpr *expr) {}
		virtual void visit(UnaryExpr *expr) {}
		virtual void visit(FnCallExpr *expr) {}
	};
}
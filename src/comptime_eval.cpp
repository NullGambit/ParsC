#include "comptime_eval.hpp"

#include "expr.hpp"
#include "stmt.hpp"


void pars::ComptimeEval::visit(VarDeclStmt *stmt, VisitCtx ctx)
{
	if (has_flag(stmt->flags, VarFlags::Const) && stmt->initializer != nullptr)
	{
		stmt->initializer->accept(this, ctx);
	}
}

void pars::ComptimeEval::visit(SymbolExpr *expr, VisitCtx ctx)
{
	expr->symbol_node->accept(this, ctx);
}

void pars::ComptimeEval::visit(LiteralExpr *expr, VisitCtx ctx)
{
	*ctx.result = expr;
}

void pars::ComptimeEval::visit(FnDecl *fn, VisitCtx ctx)
{
	for (auto *node : fn->body->nodes)
	{
		node->accept(this, ctx);
	}
}

void pars::ComptimeEval::visit(BinaryExpr *expr, VisitCtx ctx)
{
	Node *left_node;
	Node *right_node;

	expr->left->accept(this, {.result = &left_node});
	expr->left->accept(this, {.result = &right_node});

	auto *left = dynamic_cast<LiteralExpr*>(left_node);
	auto *right = dynamic_cast<LiteralExpr*>(left_node);

	if (left == nullptr || right == nullptr)
	{
		*ctx.result = nullptr;
	}

	using enum TokenType;

	switch (expr->op)
	{
		case Greater:
		{
			auto *result = new_node<LiteralExpr>();

			result->value = left->value > right->value;

			*ctx.result = result;

			break;
		}
	}
}

pars::ComptimeEval::ComptimeEval(ParseCtx *ctx)
{
	m_ctx = ctx;
}

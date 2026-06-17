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

void pars::ComptimeEval::visit(CallExpr *expr, VisitCtx ctx)
{
	auto scope = m_ctx->scope_table.new_scope();

	for (auto i = 0; auto *param : expr->prototype->signature.parameters)
	{
		m_ctx->scope_table.add_to_scope(param->symbol, expr->arguments[i]);
	}

	for (auto *node : expr->prototype->body->nodes)
	{
		node->accept(this, ctx);
	}
}

void pars::ComptimeEval::visit(BinaryExpr *expr, VisitCtx ctx)
{
	Node *left_node {};
	Node *right_node {};

	expr->left->accept(this, {.result = &left_node});
	expr->left->accept(this, {.result = &right_node});

	auto *left = dynamic_cast<LiteralExpr*>(left_node);
	auto *right = dynamic_cast<LiteralExpr*>(left_node);

	if (left == nullptr || right == nullptr)
	{
		*ctx.result = nullptr;
		return;
	}

	using enum TokenType;

	switch (expr->op)
	{
		case Greater:
		{
			auto *result = new_node<LiteralExpr>();

			//result->value = left->value > right->value;

			*ctx.result = result;

			break;
		}
	}
}

pars::ComptimeEval::ComptimeEval(ParseCtx *ctx)
{
	m_ctx = ctx;
}

#include "comp_eval.hpp"

#include "expr.hpp"
#include "frontend_error.hpp"
#include "parse_ctx.hpp"
#include "stmt.hpp"
#include "util/fmt.hpp"
#include "overload.hpp"

pars::Node* pars::CompEval::visit(BinaryExpr* expr, VisitCtx ctx)
{
    auto *left_node = expr->left->accept(this, ctx);
    auto *right_node = expr->right->accept(this, ctx);

    auto *left = dynamic_cast<LiteralExpr*>(left_node);
    auto *right = dynamic_cast<LiteralExpr*>(right_node);

    if (left == nullptr || right == nullptr)
    {
        throw FrontendError{expr->token, "Both operands of expression must evaluate at compile time"};
    }

    // TODO add a comp time version of operators in pars::Type

    return nullptr;
}

pars::Node* pars::CompEval::visit(LiteralExpr* expr, VisitCtx ctx)
{
    return expr;
}

pars::Node* pars::CompEval::visit(SymbolExpr* expr, VisitCtx ctx)
{
    auto *symbol = parse_ctx->scope_table.find_symbol(expr->symbol);

    if (symbol == nullptr)
    {
        throw FrontendError{expr->token, fmt::format("symbol {} does not exist", expr->symbol)};
    }

    return symbol->accept(this, ctx);
}

pars::Node* pars::CompEval::visit(VarDeclStmt* stmt, VisitCtx ctx)
{
    if (stmt->initializer)
    {
        return stmt->initializer->accept(this, ctx);
    }

    return nullptr;
}

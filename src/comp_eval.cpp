#include "comp_eval.hpp"

#include "expr.hpp"
#include "frontend_error.hpp"
#include "parse_ctx.hpp"
#include "stmt.hpp"
#include "util/fmt.hpp"
#include "overload.hpp"

namespace pars
{
    struct ConstantReturnStmt : Node
    {
        LiteralExprValue value;
    };
}

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

    auto ok = true;

    auto result = std::visit(
    [&ok, &op = expr->op](auto &&lhs, auto &&rhs) -> LiteralExprValue
    {
        using L = std::decay_t<decltype(lhs)>;
        using R = std::decay_t<decltype(rhs)>;

        using enum TokenType;

        if constexpr (std::is_arithmetic_v<L> && std::is_arithmetic_v<R>)
        {
            switch (op)
            {
                case Plus: return lhs + rhs;
                case Minus: return lhs - rhs;
                case Star: return lhs * rhs;
                case ForwardSlash: return lhs / rhs;
            }
        }

        ok = false;

        return {};
    }, left->value, right->value);

    if (!ok)
    {
        return nullptr;
    }

    return std::visit([](auto &&val) -> LiteralExpr*
    {
        auto literal = new_node<LiteralExpr>();

        literal->value = val;

        return literal;
    }, result);
}

pars::Node* pars::CompEval::visit(LiteralExpr* expr, VisitCtx ctx)
{
    return expr;
}

pars::Node * pars::CompEval::visit(NamedExpr *expr, VisitCtx ctx)
{
    return expr->value;
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

/*
 *  fn do_stuff(value: i32): i32
 *  {
 *      var x = value ** 2
 *      let y = -value
 *
 *      x += y
 *
 *      if x > y
 *      {
 *          return x
*       }
*
*       return y
 *  }
 *
 *  fn double(x: value) => x * 2
 *
 *  const result1 = do_stuff(100)
 *  const result2 = double(result1)
*/

pars::Node * pars::CompEval::visit(FnDecl *stmt, VisitCtx ctx)
{
    if (stmt->signature.return_type->is_equal(&VoidType))
    {
        return nullptr;
    }

    return Visitor::visit(stmt, ctx);
}

namespace pars
{
    Expr* eval_aggregate(AggregateExpr *expr, VisitCtx ctx, CompEval *eval)
    {
        for (auto &element : expr->initializers)
        {
            element.expr = dynamic_cast<Expr*>(element.expr->accept(eval, ctx));

            if (element.expr == nullptr)
            {
                return nullptr;
            }
        }

        return expr;
    }
}

pars::Node * pars::CompEval::visit(ArrayLiteralExpr *expr, VisitCtx ctx)
{
    return eval_aggregate(expr, ctx, this);
}

pars::Node * pars::CompEval::visit(StructLiteral *expr, VisitCtx ctx)
{
    return eval_aggregate(expr, ctx, this);
}

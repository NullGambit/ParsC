#pragma once

#include "visitor.hpp"

namespace pars
{
    struct CompEval : Visitor
    {
        ParseCtx *parse_ctx;

        Node* produce(BinaryExpr* expr, VisitCtx ctx) override;
        Node* produce(LiteralExpr* expr, VisitCtx ctx) override;
        Node* produce(SymbolExpr* expr, VisitCtx ctx) override;
        Node* produce(VarDeclStmt* stmt, VisitCtx ctx) override;
    };
}

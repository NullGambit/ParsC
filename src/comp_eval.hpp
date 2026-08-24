#pragma once

#include "visitor.hpp"

namespace pars
{
    struct CompEval : Visitor
    {
        ParseCtx *parse_ctx;

        Node* visit(BinaryExpr* expr, VisitCtx ctx) override;
        Node* visit(LiteralExpr* expr, VisitCtx ctx) override;
        Node* visit(SymbolExpr* expr, VisitCtx ctx) override;
        Node* visit(VarDeclStmt* stmt, VisitCtx ctx) override;
        Node *visit(FnDecl *stmt, VisitCtx ctx) override;
        Node *visit(ArrayLiteralExpr *expr, VisitCtx ctx) override;
    };
}

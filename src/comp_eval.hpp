#pragma once

#include "visitor.hpp"

namespace pars
{
    // TODO the temporary nodes created by the comp eval should be reused
    //      only the output node should use the normal allocator as the rest of the compiler
    struct CompEval : Visitor
    {
        ParseCtx *parse_ctx;

        Node* visit(BinaryExpr* expr, VisitCtx ctx) override;
        Node* visit(LiteralExpr* expr, VisitCtx ctx) override;
        Node* visit(NamedExpr* expr, VisitCtx ctx) override;
        Node* visit(SymbolExpr* expr, VisitCtx ctx) override;
        Node* visit(VarDeclStmt* stmt, VisitCtx ctx) override;
        Node* visit(FnDecl *stmt, VisitCtx ctx) override;
        Node* visit(ArrayLiteralExpr *expr, VisitCtx ctx) override;
        Node* visit(StructLiteral *expr, VisitCtx ctx) override;
        Node* visit(CallExpr *expr, VisitCtx ctx) override;
        Node* visit(ReturnStmt *stmt, VisitCtx ctx) override;
    };
}

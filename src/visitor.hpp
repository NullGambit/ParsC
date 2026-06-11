#pragma once
#include <vector>

namespace pars
{
	struct TakeAddressExpr;
	struct ContinueStmt;
	struct BreakStmt;
	struct ForStmt;
	struct CompIfStmt;
	struct WhileStmt;
	struct IfStmt;
	struct AbsExpr;
	struct AssignmentStmt;
	struct BlockStmt;
	struct NamedExpr;
	struct AnonInitExpr;
	struct CastExpr;
	struct MemberAccessExpr;
	struct TypeExpr;
	struct SizeofExpr;
	struct AliasType;
	struct FnDecl;
	struct Symbol;
	struct GroupExpr;
	struct ReturnStmt;
	struct VarDeclStmt;
	struct ImportStmt;
	struct SymbolExpr;
	struct LiteralExpr;
	struct CallExpr;
	struct UnaryExpr;
	struct BinaryExpr;
	struct Node;
	struct Type;

	struct VisitCtx
	{
		Type *type;
		Node *invoker;
		Node **result;
	};

	struct Visitor
	{
		virtual ~Visitor() = default;

		virtual void visit(BinaryExpr *expr, VisitCtx ctx) {}
		virtual void visit(UnaryExpr *expr, VisitCtx ctx) {}
		virtual void visit(CallExpr *expr, VisitCtx ctx) {}
		virtual void visit(LiteralExpr *expr, VisitCtx ctx) {}
		virtual void visit(SymbolExpr *expr, VisitCtx ctx) {}
		virtual void visit(GroupExpr *expr, VisitCtx ctx) {}
		virtual void visit(MemberAccessExpr *expr, VisitCtx ctx) {}
		virtual void visit(CastExpr *expr, VisitCtx ctx) {}
		virtual void visit(AnonInitExpr *expr, VisitCtx ctx) {}
		virtual void visit(NamedExpr *expr, VisitCtx ctx) {}
		virtual void visit(AbsExpr *expr, VisitCtx ctx) {}
		virtual void visit(TakeAddressExpr *expr, VisitCtx ctx) {}
		virtual void visit(ImportStmt *stmt, VisitCtx ctx) {}
		virtual void visit(VarDeclStmt *stmt, VisitCtx ctx) {}
		virtual void visit(ReturnStmt *stmt, VisitCtx ctx) {}
		virtual void visit(IfStmt *stmt, VisitCtx ctx) {}
		virtual void visit(CompIfStmt *stmt, VisitCtx ctx) {}
		virtual void visit(WhileStmt *stmt, VisitCtx ctx) {}
		virtual void visit(ForStmt *stmt, VisitCtx ctx) {}
		virtual void visit(BreakStmt *stmt, VisitCtx ctx) {}
		virtual void visit(ContinueStmt *stmt, VisitCtx ctx) {}
		virtual void visit(Symbol *stmt, VisitCtx ctx) {}
		virtual void visit(FnDecl *stmt, VisitCtx ctx) {}
		virtual void visit(AliasType *stmt, VisitCtx ctx) {}
		virtual void visit(SizeofExpr *stmt, VisitCtx ctx) {}
		virtual void visit(TypeExpr *stmt, VisitCtx ctx) {}
		virtual void visit(BlockStmt *stmt, VisitCtx ctx) {}
		virtual void visit(AssignmentStmt *stmt, VisitCtx ctx) {}

		void visit_nodes(const std::vector<Node*> &nodes);
	};
}

#pragma once
#include <bitset>
#include <vector>

namespace pars
{
	struct AggregateExpr;
	struct Array;
	struct BaseArray;
	struct Pointer;
	struct UnresolvedSymbol;
	struct SliceExpr;
	struct StructLiteral;
	struct Struct;
	struct ParseCtx;
	struct IndexOpExpr;
	struct ArrayLiteralExpr;
	struct PackedExpr;
	struct DereferenceExpr;
	struct PtrOpExpr;
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
		Type *type {};
		Node *invoker {};
		Node **result {};
		bool member {};
		u8 depth {};
		ParseCtx *parse_ctx_override {};
	};

	struct Visitor
	{
		virtual ~Visitor() = default;

		virtual Node* visit(BinaryExpr *expr, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(UnaryExpr *expr, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(CallExpr *expr, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(LiteralExpr *expr, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(SymbolExpr *expr, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(GroupExpr *expr, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(MemberAccessExpr *expr, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(CastExpr *expr, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(AnonInitExpr *expr, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(NamedExpr *expr, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(AbsExpr *expr, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(PtrOpExpr *expr, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(PackedExpr *expr, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(ArrayLiteralExpr *expr, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(IndexOpExpr *expr, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(StructLiteral *expr, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(SliceExpr *expr, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(AggregateExpr *expr, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(ImportStmt *stmt, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(VarDeclStmt *stmt, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(ReturnStmt *stmt, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(IfStmt *stmt, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(CompIfStmt *stmt, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(WhileStmt *stmt, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(ForStmt *stmt, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(BreakStmt *stmt, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(ContinueStmt *stmt, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(Symbol *stmt, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(FnDecl *stmt, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(Struct *stmt, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(AliasType *stmt, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(SizeofExpr *stmt, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(TypeExpr *stmt, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(BlockStmt *stmt, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(AssignmentStmt *stmt, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(UnresolvedSymbol *type, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(Pointer *type, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(BaseArray *type, VisitCtx ctx) { return nullptr; }
		virtual Node* visit(Array *type, VisitCtx ctx) { return nullptr; }

		void visit_nodes(const std::vector<Node*> &nodes);
	};
}

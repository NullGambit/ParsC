#pragma once
#include <string_view>
#include <vector>

#include "node.hpp"
#include "util/macros.hpp"

namespace pars
{
	struct Expr;

	struct Stmt : Node
	{

	};

	enum class VarFlags : u8
	{
		Const = 1 << 0,
		Static = 1 << 1,
		Volatile = 1 << 2,
	};

	PARSE_FLAGIFY(VarFlags);

	struct VarDeclStmt : Stmt
	{
		Symbol symbol;
		std::string_view type;
		Expr* initializer;
		VarFlags flags;

		ACCEPT
	};

	struct FnPrototypeStmt : Stmt
	{
		std::vector<VarDeclStmt*> parameters;
		std::string_view return_type;
		bool is_extern = false;

		ACCEPT
	};

	struct ImportStmt : Stmt
	{
		std::vector<std::string_view> path;
		std::string_view alias;
		std::vector<std::string_view> selective_imports;

		ACCEPT
	};

	struct FnStmt : Stmt
	{
		FnPrototypeStmt prototype;
		Symbol symbol;
		std::vector<Node*> body;

		ACCEPT
	};

	struct ReturnStmt : Stmt
	{
		Expr* expr {};

		ACCEPT
	};

	struct PrintlnStmt : Stmt
	{
		Expr* expr {};

		ACCEPT
	};
}

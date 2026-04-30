#pragma once
#include <string_view>
#include <vector>

#include "node.hpp"

namespace pars
{
	struct Expr;

	struct TypedSymbol
	{
		std::string_view name;
		std::string_view type;
	};

	struct FnPrototype
	{
		std::vector<TypedSymbol> parameters;
		std::string_view return_type;
	};

	struct Stmt : Node
	{

	};

	struct ImportStmt : Stmt
	{
		std::vector<std::string_view> path;
		std::string_view alias;
		std::vector<std::string_view> selective_imports;
	};

	struct FnStmt : Stmt
	{
		FnPrototype prototype;
		std::string_view symbol;
		std::vector<Node*> body;
	};

	struct VarStmt : Stmt
	{
		std::string_view symbol;
		std::string_view type;
		Expr* initializer;
	};
}

#pragma once
#include <string_view>
#include <variant>
#include <vector>

#include "node.hpp"

namespace pars
{
	struct Expr : Node
	{

	};

	using LiteralExprValue = std::variant
	<
		i32,
		f32,
		std::string_view
	>;

	struct LiteralExpr : Expr
	{
		LiteralExprValue value;
	};

	struct BinaryExpr : Expr
	{
		size_t left;
		char op;
		Expr* right;
	};

	struct UnaryExpr : Expr
	{
		char op;
		Expr* right;
	};

	// for not only variables but also functions to be used when taking the address of a function
	struct SymbolExpr : Expr
	{
		std::string_view symbol;
	};

	struct FnCallExpr : Expr
	{
		std::string_view symbol;
		std::vector<Expr*> arguments;
	};
}

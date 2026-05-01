#pragma once
#include <string_view>
#include <variant>
#include <vector>

#include "node.hpp"
#include "visitor.hpp"

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

		//NODE_BODY
	};

	struct BinaryExpr : Expr
	{
		size_t left;
		char op;
		Expr* right;

		//NODE_BODY
	};

	struct UnaryExpr : Expr
	{
		char op;
		Expr* right;

		//NODE_BODY
	};

	// for not only variables but also functions to be used when taking the address of a function
	struct SymbolExpr : Expr
	{
		std::string_view symbol;

		//NODE_BODY
	};

	struct FnCallExpr : Expr
	{
		std::string_view symbol;
		std::vector<Expr*> arguments;

		//NODE_BODY
	};
}

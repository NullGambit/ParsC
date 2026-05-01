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
		i64,
		f32,
		std::string_view
	>;

	struct LiteralExpr : Expr
	{
		LiteralExprValue value;

		ACCEPT
	};

	struct BinaryExpr : Expr
	{
		Expr* left;
		char op;
		Expr* right;

		ACCEPT
	};

	struct UnaryExpr : Expr
	{
		char op;
		Expr* right;

		ACCEPT
	};

	// for not only variables but also functions to be used when taking the address of a function
	struct SymbolExpr : Expr
	{
		std::string_view symbol;

		ACCEPT
	};

	struct FnCallExpr : Expr
	{
		std::string_view symbol;
		std::vector<Expr*> arguments;

		ACCEPT
	};

	struct GroupExpr : Expr
	{
		Expr* expr;

		ACCEPT
	};
}

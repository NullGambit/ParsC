#pragma once
#include <string_view>
#include <variant>
#include <vector>

namespace pars
{
	using LiteralExpr = std::variant
	<
		i32,
		f32,
		std::string_view
	>;

	struct BinaryExpr
	{
		size_t left;
		char op;
		size_t right;
	};

	struct UnaryExpr
	{
		char op;
		size_t right;
	};

	// for not only variables but also functions to be used when taking the address of a function
	struct SymbolExpr
	{
		std::string_view symbol;
	};

	struct FnCallExpr
	{
		std::string_view symbol;
		std::vector<size_t> arguments;
	};

	using Expression = std::variant
	<
		LiteralExpr,
		BinaryExpr,
		UnaryExpr,
		SymbolExpr,
		FnCallExpr
	>;
}

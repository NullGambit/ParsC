#pragma once
#include <string_view>
#include <variant>
#include <vector>

namespace pars
{
	struct ImportStmt
	{
		std::vector<std::string_view> path;
		std::string_view alias;
		std::vector<std::string_view> selective_imports;
	};

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

	struct FnStmt
	{
		FnPrototype prototype;
		std::string_view symbol;
		std::vector<size_t> statements;
	};

	struct VarDeclStmt
	{
		std::string_view symbol;
		std::string_view type;
		size_t initializer;
	};

	using Statement = std::variant
	<
		VarDeclStmt,
		FnStmt,
		ImportStmt
	>;

	size_t new_stmt(Statement &&statement);
	Statement& get_stmt(size_t id);
}

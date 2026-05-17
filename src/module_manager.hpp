#pragma once
#include <span>

#include "util/io.hpp"


namespace pars
{
	struct Node;
	struct Module;
	class AST;

	enum class GlobalSymbolAvailability
	{
		Always,
		WhenImported,
	};

	struct GlobalSymbol
	{
		Node *node;
		GlobalSymbolAvailability availability;
		// file (module) this symbol came from. if availability is set to always then not needed
		u32 file_id;
	};

	void add_module_path(const std::filesystem::path &path);

	Module* get_module(std::filesystem::path &path);

	void declare_global_symbol(std::string_view name, GlobalSymbol symbol);

	std::span<Module*> get_all_modules();

	std::span<GlobalSymbol> find_global_symbol(std::string_view name);
}

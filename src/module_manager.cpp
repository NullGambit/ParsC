#include "module_manager.hpp"

#include "containers/hash_map.hpp"
#include "module.hpp"
#include "util/fmt.hpp"

static pars::HashMap<std::filesystem::path, pars::Module> g_modules;

thread_local llvm::LLVMContext g_llvm_ctx;

static pars::HashMap<std::string_view, std::vector<pars::GlobalSymbol>> g_global_symbols;

pars::Module* pars::get_module(const std::filesystem::path &path)
{
	auto iter = g_modules.find(path);

	if (iter == g_modules.end())
	{
		iter = g_modules.emplace(path, Module{}).first;

		iter->second.init(path.c_str(), &g_llvm_ctx);

		auto maybe_source = load_file(path.c_str());

		if (!maybe_source.has_value())
		{
			return nullptr;
		}

		auto &source = maybe_source.value();

		iter->second.file_id = source.id;

		auto &nodes = iter->second.ast.parse(source);

		iter->second.ast.resolve_symbols();

		auto ctx = EmitCtx{&g_llvm_ctx, llvm::IRBuilder{g_llvm_ctx}, iter->second.module};

		// compile the entire module
		for (auto *node : nodes)
		{
			node->emit(ctx);
		}
	}

	return &iter->second;
}

void pars::declare_global_symbol(std::string_view name, GlobalSymbol symbol)
{
	g_global_symbols[name].emplace_back(symbol);
}

std::span<pars::GlobalSymbol> pars::find_global_symbol(std::string_view name)
{
	auto iter = g_global_symbols.find(name);

	if (iter == g_global_symbols.end())
	{
		return {};
	}

	return iter->second;
}

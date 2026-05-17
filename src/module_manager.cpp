#include "module_manager.hpp"

#include "containers/hash_map.hpp"
#include "module.hpp"
#include "util/fmt.hpp"

pars::HashMap<std::string, pars::Module*> g_modules_table;
static std::vector<pars::Module*> g_modules;

thread_local llvm::LLVMContext g_llvm_ctx;

static pars::HashMap<std::string_view, std::vector<pars::GlobalSymbol>> g_global_symbols;
static std::vector<std::filesystem::path> g_include_paths;

void pars::add_module_path(const std::filesystem::path &path)
{
	g_include_paths.emplace_back(path);
}

pars::Module* pars::get_module(std::filesystem::path &path)
{
	if (!is_directory(path) && path.extension() != "pars")
	{
		path.replace_extension("pars");
	}

	auto iter = g_modules_table.find(path);

	if (iter != g_modules_table.end())
	{
		return iter->second;
	}

	auto *module = new Module{path.c_str(), &g_llvm_ctx};

	g_modules.emplace_back(module);
	g_modules_table.emplace(path, module);

	auto maybe_source = load_file(path.c_str());

	if (!maybe_source.has_value())
	{
		return nullptr;
	}

	auto &nodes = module->ast.parse(maybe_source.value());

	module->ast.resolve_symbols();

	auto ctx = EmitCtx{&g_llvm_ctx, llvm::IRBuilder{g_llvm_ctx}, module->module};

	// compile the entire module
	for (auto *node : nodes)
	{
		node->emit(ctx);
	}

	ctx.module->print(llvm::outs(), nullptr);

	return module;
}

void pars::declare_global_symbol(std::string_view name, GlobalSymbol symbol)
{
	g_global_symbols[name].emplace_back(symbol);
}

std::span<pars::Module*> pars::get_all_modules()
{
	return g_modules;
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

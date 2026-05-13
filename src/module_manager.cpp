#include "module_manager.hpp"

#include "containers/hash_map.hpp"
#include "module.hpp"
#include "util/fmt.hpp"

static pars::HashMap<std::filesystem::path, pars::Module> g_modules;

thread_local llvm::LLVMContext g_llvm_ctx;

pars::Module* pars::get_module(const std::filesystem::path &path)
{
	auto iter = g_modules.find(path);

	if (iter == g_modules.end())
	{
		iter = g_modules.emplace(path, Module{}).first;

		iter->second.init(path.c_str(), &g_llvm_ctx);

		auto source = load_file(path.c_str());

		if (!source.has_value())
		{
			return nullptr;
		}

		auto &nodes = iter->second.ast.parse(source.value());

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

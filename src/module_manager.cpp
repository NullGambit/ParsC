#include "module_manager.hpp"

#include <llvm/IR/Verifier.h>

#include "containers/hash_map.hpp"
#include "module.hpp"
#include "analyzer.hpp"
#include "compile_error.hpp"
#include "parse_ctx.hpp"
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

enum class ModulePlatform
{
	Linux,
	Windows,
	Darwin,
};

#if defined(__linux__)
ModulePlatform PLATFORM = ModulePlatform::Linux;
#elif defined(__WIN64__)
ModulePlatform PLATFORM = ModulePlatform::Windows;
#endif

pars::Module* pars::get_module(std::filesystem::path &path)
{
	std::optional<SourceFile> maybe_source;

	std::filesystem::path potential_path;

	for (auto &include_path : g_include_paths)
	{
		potential_path = include_path / path;

		if (is_directory(path))
		{
			potential_path /= "package.pars";
		}

		// TODO: use llvm target platform so this works with cross compilation
		// TODO: the performance here is sucks. lots of room for optimizations.

		auto p = potential_path.string();

		if (std::filesystem::exists(p + ".linux.pars") && PLATFORM == ModulePlatform::Linux)
		{
			potential_path.replace_extension(".linux.pars");
		}
		else if (std::filesystem::exists(p + ".windows.pars") && PLATFORM == ModulePlatform::Windows)
		{
			potential_path.replace_extension(".windows.pars");
		}

		if (!is_directory(potential_path) && potential_path.extension() != "pars")
		{
			potential_path.replace_extension("pars");
		}

		auto iter = g_modules_table.find(potential_path);

		if (iter != g_modules_table.end())
		{
			return iter->second;
		}

		maybe_source = load_file(potential_path.c_str());

		if (maybe_source.has_value())
		{
			break;
		}
	}

	if (!maybe_source.has_value())
	{
		return nullptr;
	}

	auto *module = new Module{potential_path.c_str(), &g_llvm_ctx};

	g_modules.emplace_back(module);
	g_modules_table.emplace(potential_path, module);

	auto *parse_ctx = new ParseCtx
	{
		.scope_table = {},
		.source_file = maybe_source.value(),
	};

	parse_ctx->scope_table.set_file_id(parse_ctx->source_file.id);

	auto &nodes = module->ast.parse(parse_ctx);

	auto analyzer = Analyzer{parse_ctx};

	analyzer.analyze(nodes);

	auto ctx = module->make_ctx();

	ctx.builder.SetInsertPoint((llvm::BasicBlock*)nullptr);

	// compile the entire module
	for (auto *node : nodes)
	{
		node->emit(ctx);
	}

	ctx.module->print(llvm::outs(), nullptr);

	std::string error_str;
	llvm::raw_string_ostream error_stream(error_str);

	auto has_error = llvm::verifyModule(*ctx.module, &error_stream);

	if (has_error)
	{
		// print error to the bottom of the module ir
		// otherwise easy to miss verification errors
		std::string module_str;
		llvm::raw_string_ostream module_stream(module_str);

		ctx.module->print(module_stream, nullptr);

		module_str += error_str;

		error_stream.flush();

		return nullptr;

		//throw CompileError{this, std::move(module_str)};
	}

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

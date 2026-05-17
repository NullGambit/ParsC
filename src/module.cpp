#include "module.hpp"

pars::Module::Module(std::string_view name, llvm::LLVMContext *ctx) :
	ctx{ctx},
	module{new llvm::Module{name, *ctx}}
{}

pars::EmitCtx pars::Module::make_ctx()
{
	return {ctx, llvm::IRBuilder{*ctx}, module};
}

bool pars::Module::is_equal(const Module &other) const
{
	return ast.get_file_id() == other.ast.get_file_id();
}

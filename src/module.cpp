#include "module.hpp"

void pars::Module::init(std::string_view name, llvm::LLVMContext *ctx)
{
	this->ctx = ctx;
	// its not really a memory leak since a module will live as long as the compiler is running
	// because you'll never know when this module will be imported again
	this->module = new llvm::Module{name, *ctx};
}

pars::EmitCtx pars::Module::make_ctx()
{
	return {ctx, llvm::IRBuilder{*ctx}, module};
}

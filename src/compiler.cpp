#include "compiler.hpp"

void pars::Compiler::visit(FnPrototypeStmt *fn)
{
	fn->emit(ctx);
}

void pars::Compiler::visit(BlockStmt *fn)
{
	fn->emit(ctx);
}

#include "llvm_utils.hpp"

#include <llvm/IR/Constants.h>
#include "emit_context.hpp"

thread_local llvm::PoisonValue *g_block_poison = nullptr;

llvm::Value * pars::get_block_poison(EmitCtx &ctx)
{
	if (g_block_poison == nullptr)
	{
		auto *type = llvm::Type::getVoidTy(*ctx.llvm_ctx);

		g_block_poison = llvm::PoisonValue::get(type);
	}

	return g_block_poison;
}

bool pars::is_block_poison(EmitCtx &ctx, llvm::Value *value)
{
	return value == get_block_poison(ctx);
}

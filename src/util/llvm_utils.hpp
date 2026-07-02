#pragma once
#include <llvm/IR/IRBuilder.h>

namespace llvm
{
	class Value;
}

namespace pars
{
	struct EmitCtx;

	llvm::Value *get_block_poison(EmitCtx &ctx);

	bool is_block_poison(EmitCtx &ctx, llvm::Value *value);

	llvm::IRBuilder<> get_alloca_builder(EmitCtx &ctx);
}

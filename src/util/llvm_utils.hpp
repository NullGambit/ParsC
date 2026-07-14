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

	llvm::MDNode* get_metadata(llvm::Value *value, llvm::StringRef kind);

	void set_metadata(llvm::LLVMContext *ctx, llvm::Value *value, llvm::MDTuple *md_tuple, llvm::StringRef kind);
}

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
	llvm::AllocaInst* create_alloca(EmitCtx &ctx, llvm::Type *type, const llvm::Twine &name = "");

	llvm::MDNode* get_metadata(llvm::Value *value, llvm::StringRef kind);

	void set_metadata(llvm::LLVMContext *ctx, llvm::Value *value, llvm::MDTuple *md_tuple, llvm::StringRef kind);

    bool safe_to_terminate(EmitCtx &ctx);
	llvm::BranchInst* create_safe_br(EmitCtx &ctx, llvm::BasicBlock *current_block, llvm::BasicBlock *next_block);
    llvm::ReturnInst* create_safe_void_ret(EmitCtx &ctx);
    llvm::ReturnInst* create_safe_ret(EmitCtx &ctx, llvm::Value *value);
}

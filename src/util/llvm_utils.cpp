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

llvm::IRBuilder<> pars::get_alloca_builder(EmitCtx &ctx)
{
	auto *fn = ctx.builder.GetInsertBlock()->getParent();
	auto &entry = fn->getEntryBlock();
	auto insert_point = entry.getFirstNonPHIOrDbgOrAlloca();

	return {&entry, insert_point};
}

llvm::AllocaInst * pars::create_alloca(EmitCtx &ctx, llvm::Type *type, const llvm::Twine &name)
{
	return get_alloca_builder(ctx).CreateAlloca(type, nullptr, name);
}

llvm::MDNode * pars::get_metadata(llvm::Value *value, llvm::StringRef kind)
{
	if (auto *inst = llvm::dyn_cast<llvm::Instruction>(value))
	{
		return inst->getMetadata(kind);
	}
	if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(value))
	{
		return alloca->getMetadata(kind);
	}
	if (auto *global = llvm::dyn_cast<llvm::GlobalObject>(value))
	{
		return global->getMetadata(kind);
	}

	return nullptr;
}

void pars::set_metadata(llvm::LLVMContext *ctx, llvm::Value *value, llvm::MDTuple *md_tuple, llvm::StringRef kind)
{
	auto const_md = ctx->getMDKindID(kind);

	if (auto *inst = llvm::dyn_cast<llvm::Instruction>(value))
	{
		inst->setMetadata(const_md, md_tuple);
	}
	else if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(value))
	{
		alloca->setMetadata(const_md, md_tuple);
	}
	else if (auto *global = llvm::dyn_cast<llvm::GlobalObject>(value))
	{
		global->setMetadata(const_md, md_tuple);
	}
}

llvm::BranchInst * pars::create_safe_br(EmitCtx &ctx, llvm::BasicBlock *current_block, llvm::BasicBlock *next_block)
{
	if (current_block != nullptr && current_block->getTerminator() == nullptr)
	{
		return ctx.builder.CreateBr(next_block);
	}

	return nullptr;
}

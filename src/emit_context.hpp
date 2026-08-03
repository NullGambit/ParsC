#pragma once

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include <memory>

#include "containers/hash_map.hpp"

namespace pars
{
	struct LoopBasicBlocks
	{
		llvm::BasicBlock *merge;
		llvm::BasicBlock *start;
	};

	struct EmitCtx
	{
		llvm::LLVMContext *llvm_ctx;
		llvm::IRBuilder<> builder;
		llvm::Module *module;
		HashMap<std::string_view, llvm::Value*> named_values;
		std::vector<LoopBasicBlocks> loop_bbs;
		std::vector<llvm::BasicBlock*> if_merge_bbs;
		std::vector<llvm::BasicBlock*> while_merge_bbs;
	};

	struct EmitParams
	{
		// the target ptr this emit call should write to. useful for array and struct literals. always can be nullptr.
		llvm::Value *target_ptr {};
		// the predecessor of the current emit call such as a chained member access
		llvm::Value *predecessor_ptr {};
	};
}

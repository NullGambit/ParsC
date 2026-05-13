#pragma once

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include <memory>

#include "containers/hash_map.hpp"

namespace pars
{
	struct EmitCtx
	{
		llvm::LLVMContext *llvm_ctx;
		llvm::IRBuilder<> builder;
		llvm::Module *module;
		HashMap<std::string_view, llvm::Value*> named_values;
	};
}

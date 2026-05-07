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
		std::unique_ptr<llvm::LLVMContext> llvm_ctx;
		std::unique_ptr<llvm::IRBuilder<>> builder;
		std::unique_ptr<llvm::Module> module;
		HashMap<std::string_view, llvm::Value*> named_values;
	};
}

#pragma once
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

#include "ast.hpp"

namespace llvm
{
	class LLVMContext;
}

namespace pars
{
	struct Module
	{
		llvm::LLVMContext *ctx;
		llvm::Module *module;
		AST ast;

		Module(std::string_view name, llvm::LLVMContext *ctx);

		EmitCtx make_ctx();

		[[nodiscard]]
		bool is_equal(const Module &other) const;
	};
}

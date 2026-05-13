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
		u32 file_id;

		void init(std::string_view name, llvm::LLVMContext *ctx);

		EmitCtx make_ctx();

		bool is_equal(const Module &other);
	};
}

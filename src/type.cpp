#include "type.hpp"

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Type.h>

llvm::Type * pars::Void::get_llvm_type(llvm::LLVMContext *ctx) const
{
	return llvm::Type::getVoidTy(*ctx);
}

llvm::Type * pars::Integral::get_llvm_type(llvm::LLVMContext *ctx) const
{
	return llvm::IntegerType::get(*ctx, bits);
}

llvm::Type * pars::Integer::get_llvm_type(llvm::LLVMContext *ctx) const
{
	return Integral::get_llvm_type(ctx);
}

llvm::Type * pars::Float::get_llvm_type(llvm::LLVMContext *ctx) const
{
	return Integral::get_llvm_type(ctx);
}

llvm::Type * pars::Bool::get_llvm_type(llvm::LLVMContext *ctx) const
{
	return Integral::get_llvm_type(ctx);
}

llvm::Type * pars::Char::get_llvm_type(llvm::LLVMContext *ctx) const
{
	return Integral::get_llvm_type(ctx);
}

llvm::Type * pars::Str::get_llvm_type(llvm::LLVMContext *ctx) const
{
	return llvm::PointerType::get(llvm::Type::getInt8Ty(*ctx), 0);
}


#include "type.hpp"

#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Type.h>

bool pars::check_type_equality(Type const *a_type, Type const *b_type)
{
	return a_type->is_equal(b_type) || b_type->is_equal(a_type);
}

llvm::Type * pars::Void::get_llvm_type(llvm::LLVMContext *ctx) const
{
	return llvm::Type::getVoidTy(*ctx);
}

llvm::Value * pars::Void::get_default_value(llvm::LLVMContext *ctx)
{
	return llvm::UndefValue::get(get_llvm_type(ctx));
}

llvm::Type * pars::Integral::get_llvm_type(llvm::LLVMContext *ctx) const
{
	return llvm::IntegerType::get(*ctx, bits);
}

llvm::Value * pars::Integral::get_default_value(llvm::LLVMContext *ctx)
{
	return llvm::ConstantInt::get(*ctx, llvm::APInt(bits, 0));
}

u32 pars::Integral::get_size()
{
	return bits / 8;
}

llvm::Type * pars::AliasType::get_llvm_type(llvm::LLVMContext *ctx) const
{
	return type->get_llvm_type(ctx);
}

std::string_view pars::AliasType::get_type_name() const
{
	return symbol.name;
}

bool pars::AliasType::is_equal(Type const *other) const
{
	if (is_distinct)
	{
		auto *other_alias = types_match<AliasType>(this, other);

		if (other_alias)
		{
			return type->is_equal(other_alias->type);
		}

		return false;
	}
	return true;
}

u32 pars::AliasType::get_size()
{
	return type->get_size();
}

llvm::Value * pars::AliasType::get_default_value(llvm::LLVMContext *ctx)
{
	return type->get_default_value(ctx);
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

llvm::Value * pars::Str::get_default_value(llvm::LLVMContext *ctx)
{
	auto *ptr_type = get_llvm_type(ctx);
	return llvm::ConstantPointerNull::get((llvm::PointerType*)ptr_type);
}


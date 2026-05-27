#include "type.hpp"

#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Type.h>

#include <cmath>

#include "emit_context.hpp"

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
	return llvm::ConstantInt::get(*ctx, llvm::APInt(bits, 0, is_signed));
}

llvm::Value* pars::Integral::get_property(llvm::LLVMContext* ctx, std::string_view name)
{
	auto is_min = false;

	if (name == "min")
	{
		is_min = true;
	}
	else if (name != "max")
	{
		return nullptr;
	}

	auto modified_bits = bits;

	if (is_signed)
	{
		modified_bits--;
	}

	auto value = (u64)std::pow(2, modified_bits) - 1;

	if (is_signed && is_min)
	{
		value = -value;
	}
	else if (is_min)
	{
		value = 0;
	}

	return llvm::ConstantInt::get(*ctx, llvm::APInt(bits, value, is_signed));
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

llvm::Value * pars::Integer::op_binary(EmitCtx &ctx, TokenType op, llvm::Value *lhs, llvm::Value *rhs) const
{
	using enum TokenType;

	switch (op)
	{
		case Plus: return ctx.builder.CreateAdd(lhs, rhs);
		case Minus: return ctx.builder.CreateSub(lhs, rhs);
		case Star: return ctx.builder.CreateMul(lhs, rhs);
		case StarStar:
		{
			auto to_float = [&ctx, this](llvm::Value *value)
			{
				[[likely]]
				if (value->getType()->isIntegerTy())
				{
					auto *type = llvm::Type::getFloatTy(*ctx.llvm_ctx);
					return is_signed ? ctx.builder.CreateSIToFP(value, type) : ctx.builder.CreateUIToFP(value, type);
				}

				return value;
			};

			lhs = to_float(lhs);
			rhs = to_float(rhs);

			// should probably use powi but causes a crash despite any type casts
			auto *result = ctx.builder.CreateBinaryIntrinsic(llvm::Intrinsic::pow, lhs, rhs);

			if (is_signed)
			{
				return ctx.builder.CreateFPToSI(result, get_llvm_type(ctx.llvm_ctx));
			}

			return ctx.builder.CreateFPToUI(result, get_llvm_type(ctx.llvm_ctx));
		}
		case ForwardSlash: return is_signed ? ctx.builder.CreateSDiv(lhs, rhs) : ctx.builder.CreateUDiv(lhs, rhs);
		case Percent: return is_signed ? ctx.builder.CreateSRem(lhs, rhs) : ctx.builder.CreateURem(lhs, rhs);
		default: return nullptr;
	}
}

llvm::Value * pars::Integer::op_unary(EmitCtx &ctx, TokenType op, llvm::Value *rhs) const
{
	if (op == TokenType::Minus)
	{
		return ctx.builder.CreateNeg(rhs);
	}

	return nullptr;
}

llvm::Type * pars::Float::get_llvm_type(llvm::LLVMContext *ctx) const
{
	return llvm::Type::getFloatTy(*ctx);
}

llvm::Value * pars::Float::op_binary(EmitCtx &ctx, TokenType op, llvm::Value *lhs, llvm::Value *rhs) const
{
	using enum TokenType;

	switch (op)
	{
		case Plus: return ctx.builder.CreateFAdd(lhs, rhs);
		case Minus: return ctx.builder.CreateFSub(lhs, rhs);
		case ForwardSlash: return ctx.builder.CreateFDiv(lhs, rhs);
		case Star: return ctx.builder.CreateFMul(lhs, rhs);
		case Percent: return ctx.builder.CreateFRem(lhs, rhs);
		default: return nullptr;
	}
}

llvm::Value * pars::Float::op_unary(EmitCtx &ctx, TokenType op, llvm::Value *rhs) const
{
	if (op == TokenType::Minus)
	{
		return ctx.builder.CreateFNeg(rhs);
	}

	return nullptr;
}

llvm::Type * pars::Bool::get_llvm_type(llvm::LLVMContext *ctx) const
{
	return Integral::get_llvm_type(ctx);
}

llvm::Value * pars::Bool::op_binary(EmitCtx &ctx, TokenType op, llvm::Value *lhs, llvm::Value *rhs) const
{
	using enum TokenType;

	auto *type = lhs->getType();

	if (type->isIntegerTy())
	{
		switch (op)
		{
			case EqualEqual: return ctx.builder.CreateICmpEQ(lhs, rhs);
			case BangEqual: return ctx.builder.CreateICmpNE(lhs, rhs);
			case GreaterEqual: return ctx.builder.CreateICmpSGT(lhs, rhs);
			case LessEqual: return ctx.builder.CreateICmpSLE(lhs, rhs);
			case Less: return ctx.builder.CreateICmpSLT(lhs, rhs);
			case Greater: return ctx.builder.CreateICmpSGT(lhs, rhs);
		}
	}

	if (type->isFloatTy())
	{
		switch (op)
		{
			case EqualEqual: return ctx.builder.CreateFCmpOEQ(lhs, rhs);
			case BangEqual: return ctx.builder.CreateFCmpONE(lhs, rhs);
			case GreaterEqual: return ctx.builder.CreateFCmpOGE(lhs, rhs);
			case LessEqual: return ctx.builder.CreateFCmpOLE(lhs, rhs);
			case Less: return ctx.builder.CreateFCmpOLT(lhs, rhs);
			case Greater: return ctx.builder.CreateFCmpOGT(lhs, rhs);
		}
	}

	switch (op)
	{
		case And: return ctx.builder.CreateLogicalAnd(lhs, rhs);
		case Or: return ctx.builder.CreateLogicalOr(lhs, rhs);
		default: return nullptr;
	}
}

llvm::Value * pars::Bool::op_unary(EmitCtx &ctx, TokenType op, llvm::Value *rhs) const
{
	if (op == TokenType::Bang)
	{
		return ctx.builder.CreateNot(rhs);
	}

	return nullptr;
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


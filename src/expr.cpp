#include "expr.hpp"

#include "compile_error.hpp"
#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalVariable.h"

#include "overload.hpp"

#include "llvm/ADT/APFloat.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "util/fmt.hpp"

llvm::Value * pars::LiteralExpr::emit(EmitCtx &ctx)
{
	return std::visit(ccc::overload
	{
		[&ctx](i32 _i32)
		{
			auto value = llvm::APInt(32, _i32);
			return (llvm::Value*)llvm::ConstantInt::get(*ctx.llvm_ctx, value);
		},
		[&ctx](f32 _f32)
		{
			auto value = llvm::APFloat(_f32);
			return (llvm::Value*)llvm::ConstantFP::get(*ctx.llvm_ctx, value);
		},
		[&ctx](std::string_view str)
		{
			return (llvm::Value*)ctx.builder->CreateGlobalString(str, ".str", 0, ctx.module.get());
		},
		[&ctx](bool _bool)
		{
			auto value = llvm::APInt(8, _bool);
			return (llvm::Value*)llvm::ConstantInt::get(*ctx.llvm_ctx, value);
		}
	}, value);
}

llvm::Value * pars::BinaryExpr::emit(EmitCtx &ctx)
{
	auto lhs = left->emit(ctx);
	auto rhs = right->emit(ctx);

	switch (op)
	{
		case '+': return ctx.builder->CreateAdd(lhs, rhs);
		case '-': return ctx.builder->CreateSub(lhs, rhs);
		case '/': return ctx.builder->CreateFDiv(lhs, rhs);
		case '*': return ctx.builder->CreateMul(lhs, rhs);
	}
}

llvm::Value * pars::UnaryExpr::emit(EmitCtx &ctx)
{
	auto rhs = right->emit(ctx);

	switch (op)
	{
		case '-': return ctx.builder->CreateNeg(rhs);
	}
}

llvm::Value * pars::SymbolExpr::emit(EmitCtx &ctx)
{
	return ctx.named_values[symbol];
}

llvm::Value * pars::CallExpr::emit(EmitCtx &ctx)
{
	auto *fn = ctx.module->getFunction(symbol);

	if (fn->arg_size() != arguments.size())
	{
		throw CompileError{this, fmt::format("expected {} argument but got {}", fn->arg_size(), arguments.size())};
	}

	std::vector<llvm::Value*> argv;

	argv.reserve(fn->arg_size());

	for (auto *arg : arguments)
	{
		argv.emplace_back(arg->emit(ctx));
	}

	return ctx.builder->CreateCall(fn, argv);
}

llvm::Value * pars::GroupExpr::emit(EmitCtx &ctx)
{
	return expr->emit(ctx);
}

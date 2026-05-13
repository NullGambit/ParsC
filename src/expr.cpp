#include "expr.hpp"

#include "compile_error.hpp"
#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalVariable.h"

#include "overload.hpp"
#include "stmt.hpp"
#include "type.hpp"

#include "llvm/ADT/APFloat.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "util/fmt.hpp"

static pars::HashMap<std::string_view, llvm::GlobalVariable*> g_static_strings;

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
			auto iter = g_static_strings.find(str);

			if (iter != g_static_strings.end())
			{
				return (llvm::Value*)iter->second;
			}

			auto *global = ctx.builder.CreateGlobalString(str, ".str", 0, ctx.module);

			g_static_strings[str] = global;

			return (llvm::Value*)global;
		},
		[&ctx](bool _bool)
		{
			auto value = llvm::APInt(8, _bool);
			return (llvm::Value*)llvm::ConstantInt::get(*ctx.llvm_ctx, value);
		},
		[&ctx](char _char)
		{
			auto value = llvm::APInt(8, _char);
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
		case '+': return ctx.builder.CreateAdd(lhs, rhs);
		case '-': return ctx.builder.CreateSub(lhs, rhs);
		case '/': return ctx.builder.CreateFDiv(lhs, rhs);
		case '*': return ctx.builder.CreateMul(lhs, rhs);
	}
}

llvm::Value * pars::UnaryExpr::emit(EmitCtx &ctx)
{
	auto rhs = right->emit(ctx);

	switch (op)
	{
		case '-': return ctx.builder.CreateNeg(rhs);
	}
}

llvm::Value * pars::SymbolExpr::emit(EmitCtx &ctx)
{
	return ctx.named_values[symbol];
}

llvm::Value * pars::CallExpr::emit(EmitCtx &ctx)
{
	auto *fn = ctx.module->getFunction(symbol);

	if (fn == nullptr)
	{
		fn = (llvm::Function*)prototype->emit(ctx);
	}

	if (fn->arg_size() != arguments.size())
	{
		throw CompileError{this, fmt::format("expected {} argument but got {}", fn->arg_size(), arguments.size())};
	}

	std::vector<llvm::Value*> argv;

	argv.reserve(fn->arg_size());

	for (auto i = 0; auto *arg : arguments)
	{
		auto *desired_type = prototype->parameters[i]->type;

		if (!check_type_equality(arg->type, desired_type))
		{
			throw CompileError
			{
				this,
				fmt::format
				(
					"expected type {} instead of {} at position {}",
					desired_type->get_type_name(),
					arg->type->get_type_name(),
					i
				)
			};
		}

		argv.emplace_back(arg->emit(ctx));

		i++;
	}

	return ctx.builder.CreateCall(fn, argv);
}

llvm::Value * pars::GroupExpr::emit(EmitCtx &ctx)
{
	return expr->emit(ctx);
}

llvm::Value * pars::SizeofExpr::emit(EmitCtx &ctx)
{
	auto value = llvm::APInt(32, expr->type->get_size());
	return llvm::ConstantInt::get(*ctx.llvm_ctx, value);
}

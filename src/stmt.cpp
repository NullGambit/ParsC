#include "stmt.hpp"

#include <iostream>
#include <llvm/IR/Verifier.h>

#include "compile_error.hpp"
#include "expr.hpp"
#include "type.hpp"
#include "util/fmt.hpp"

llvm::Value* pars::FnPrototypeStmt::emit(EmitCtx &ctx)
{
	std::vector<llvm::Type*> param_types;

	param_types.reserve(parameters.size());

	auto *llvm_ctx = ctx.llvm_ctx.get();

	for (auto *param : parameters)
	{
		param_types.emplace_back(param->type->get_llvm_type(llvm_ctx));
	}

	auto *ft = llvm::FunctionType::get(return_type->get_llvm_type(llvm_ctx), param_types, false);

	auto *fn = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, symbol.name, ctx.module.get());

	for (auto i = 0; auto &arg : fn->args())
	{
		arg.setName(parameters[i++]->symbol.name);
	}

	return fn;
}

llvm::Function* setup_function(pars::FnPrototypeStmt *prototype, pars::EmitCtx &ctx)
{
	auto *fn = ctx.module->getFunction(prototype->symbol.name);

	if (fn == nullptr)
	{
		fn = static_cast<llvm::Function*>(prototype->emit(ctx));
	}

	if (!fn->empty())
	{
		throw pars::CompileError{prototype, fmt::format("Function {} is already defined", prototype->symbol.name)};
	}

	auto *bb = llvm::BasicBlock::Create(*ctx.llvm_ctx, "entry", fn);

	ctx.builder->SetInsertPoint(bb);

	ctx.named_values.clear();

	for (auto &arg : fn->args())
	{
		ctx.named_values[arg.getName()] = &arg;
	}

	return fn;
}

llvm::Value* pars::BlockStmt::emit(EmitCtx &ctx)
{
	auto *fn = setup_function(owner, ctx);

	for (auto *entry : body)
	{
		if (auto *expr = dynamic_cast<Expr*>(entry))
		{
			expr->emit(ctx);
		}
		else if (auto *ret = dynamic_cast<ReturnStmt*>(entry))
		{
			ret->emit(ctx);
		}
	}

	if (owner->return_type->is_equal(&VoidType))
	{
		ctx.builder->CreateRetVoid();
	}

	std::string error_str;
	llvm::raw_string_ostream error_stream(error_str);

	auto has_error = llvm::verifyFunction(*fn, &error_stream);

	if (has_error)
	{
		error_stream.flush();
		fn->eraseFromParent();
		throw CompileError{this, std::move(error_str)};
	}

	return fn;
}

llvm::Value * pars::ExprFnStmt::emit(EmitCtx &ctx)
{
	auto *fn = setup_function(owner, ctx);

	ctx.builder->CreateRet(expr->emit(ctx));

	return fn;
}

// TODO handle mismatched return types
llvm::Value * pars::ReturnStmt::emit(EmitCtx &ctx)
{
	if (expr == nullptr)
	{
		return ctx.builder->CreateRetVoid();
	}

	return ctx.builder->CreateRet(expr->emit(ctx));
}

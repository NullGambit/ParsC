#include "stmt.hpp"

#include <iostream>
#include <llvm/IR/Verifier.h>

#include "CompileError.hpp"
#include "expr.hpp"
#include "util/fmt.hpp"

llvm::Function* pars::FnPrototypeStmt::emit(EmitCtx &ctx)
{
	llvm::Type *ret_type = nullptr;

	static auto i32_type = llvm::Type::getInt32Ty(*ctx.llvm_ctx);

	if (return_type == "i32")
	{
		ret_type = i32_type;
	}
	else
	{
		ret_type = llvm::Type::getVoidTy(*ctx.llvm_ctx);
	}

	std::vector<llvm::Type*> param_types;

	param_types.reserve(parameters.size());

	for (auto *param : parameters)
	{
		if (param->type == "i32")
		{
			param_types.emplace_back(i32_type);
		}
	}

	auto *ft = llvm::FunctionType::get(ret_type, param_types, false);

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
		fn = prototype->emit(ctx);
	}

	if (!fn->empty())
	{
		throw pars::CompileError{fmt::format("Function {} is already defined", prototype->symbol.name)};
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

llvm::Function* pars::BlockStmt::emit(EmitCtx &ctx)
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

	if (owner->return_type == "void")
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
		throw CompileError{std::move(error_str)};
	}

	return fn;
}

llvm::Function * pars::ExprFnStmt::emit(EmitCtx &ctx)
{
	auto *fn = setup_function(owner, ctx);

	ctx.builder->CreateRet(expr->emit(ctx));

	return fn;
}

llvm::Value * pars::ReturnStmt::emit(EmitCtx &ctx)
{
	return ctx.builder->CreateRet(expr->emit(ctx));
}

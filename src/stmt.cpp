#include "stmt.hpp"

#include <iostream>
#include <llvm/IR/Verifier.h>

#include "compile_error.hpp"
#include "expr.hpp"
#include "type.hpp"
#include "debug/ast_printer.hpp"
#include "util/fmt.hpp"

llvm::Value * pars::VarDeclStmt::emit(EmitCtx &ctx)
{
	llvm::Value *value;

	if (initializer == nullptr)
	{
		value = type->get_default_value(ctx.llvm_ctx);
	}
	else
	{
		value = initializer->emit(ctx);
	}

	ctx.named_values[symbol.name] = value;

	return value;
}

llvm::Function * pars::FnSignature::emit(EmitCtx &ctx, std::string_view name, FnFlags flags)
{
	if (auto *fn = ctx.module->getFunction(name))
	{
		return fn;
	}

	std::vector<llvm::Type*> param_types;

	param_types.reserve(parameters.size());

	auto *llvm_ctx = ctx.llvm_ctx;

	for (auto *param : parameters)
	{
		param_types.emplace_back(param->type->get_llvm_type(llvm_ctx));
	}

	auto *ft = llvm::FunctionType::get(return_type->get_llvm_type(llvm_ctx), param_types, false);

	auto linkage = llvm::Function::InternalLinkage;

	if (has_flag(flags, FnFlags::Extern) || !has_flag(flags, FnFlags::Private))
	{
		linkage = llvm::Function::ExternalLinkage;
	}

	auto *fn = llvm::Function::Create(ft, linkage, name, ctx.module);

	if (has_flag(flags, FnFlags::Inline))
	{
		fn->addFnAttr(llvm::Attribute::AlwaysInline);
	}

	return fn;
}

llvm::Value* pars::FnDecl::emit(EmitCtx &ctx)
{
	auto *fn = signature.emit(ctx, symbol.name, flags);

	for (auto i = 0; auto &arg : fn->args())
	{
		arg.setName(signature.parameters[i++]->symbol.name);
	}

	if (!has_flag(flags, FnFlags::Extern))
	{
		auto *bb = llvm::BasicBlock::Create(*ctx.llvm_ctx, "entry", fn);

		ctx.builder.SetInsertPoint(bb);

		for (auto &arg : fn->args())
		{
			ctx.named_values[arg.getName()] = &arg;
		}

		for (auto *entry : body)
		{
			entry->emit(ctx);
		}

		if (signature.return_type->is_equal(&VoidType))
		{
			ctx.builder.CreateRetVoid();
		}
	}

	std::string error_str;
	llvm::raw_string_ostream error_stream(error_str);

	auto has_error = llvm::verifyFunction(*fn, &error_stream);

	if (has_error)
	{
		// ctx.module->print(error_stream, nullptr);
		error_stream.flush();
		fn->eraseFromParent();
		throw CompileError{this, std::move(error_str)};
	}

	return fn;
}

// TODO handle mismatched return types
llvm::Value* pars::ReturnStmt::emit(EmitCtx &ctx)
{
	if (expr == nullptr)
	{
		return ctx.builder.CreateRetVoid();
	}

	return ctx.builder.CreateRet(expr->emit(ctx));
}

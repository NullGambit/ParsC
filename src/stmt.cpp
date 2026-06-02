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

	if (!has_flag(flags, VarFlags::Const) && has_flag(flags, VarFlags::ShouldAlloca))
	{
		auto *inst = ctx.builder.CreateAlloca(type->get_llvm_type(ctx.llvm_ctx), nullptr, symbol.name);

		ctx.builder.CreateStore(value, inst, has_flag(flags, VarFlags::Volatile));

		value = inst;

		// value = ctx.builder.CreateLoad(type->get_llvm_type(ctx.llvm_ctx), inst);
	}

	ctx.named_values[symbol.name] = value;

	return value;
}

llvm::Value* pars::AssignmentStmt::emit(EmitCtx &ctx)
{
	auto *value = ctx.named_values[symbol];

	if (value == nullptr || !llvm::isa<llvm::AllocaInst>(value))
	{
		throw CompileError{this, "Cannot mutate left hand side"};
	}

	using enum TokenType;

	if (op == Equal)
	{
		return ctx.builder.CreateStore(rhs->emit(ctx), value, has_flag(lhs->flags, VarFlags::Volatile));
	}

	auto *curr_value = ctx.builder.CreateLoad(lhs->type->get_llvm_type(ctx.llvm_ctx), value);

	TokenType op_op {};

	switch (op)
	{
		case PlusEqual: op_op = Plus; break;
		case MinusEqual: op_op = Minus; break;
		case StarEqual: op_op = Star; break;
		case SlashEqual: op_op = ForwardSlash; break;
		default: return nullptr;
	}

	auto *new_value = lhs->type->op_binary(ctx, op_op, curr_value, rhs->emit(ctx));

	if (new_value == nullptr)
	{
		throw CompileError{rhs, fmt::format("this operation is not defined for type of {}", lhs->type->get_type_name())};
	}

	return ctx.builder.CreateStore(new_value, value);
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

llvm::Value* pars::BlockStmt::emit(EmitCtx &ctx)
{
	auto *bb = ctx.builder.GetInsertBlock();

	for (auto *node : nodes)
	{
		// set insert point back to this functions basic block in case
		// there is a local function being emitted
		// ctx.builder.SetInsertPoint(bb);
		auto *value = node->emit(ctx);

		if (auto *new_bb = llvm::dyn_cast_if_present<llvm::BasicBlock>(value))
		{
			bb = new_bb;
		}

		ctx.builder.SetInsertPoint(bb);
	}

	return nullptr;
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

		if (has_flag(flags, FnFlags::ArrowFn))
		{
			ctx.builder.CreateRet(body->nodes.front()->emit(ctx));
		}
		else
		{
			body->emit(ctx);

			if (signature.return_type->is_equal(&VoidType))
			{
				ctx.builder.CreateRetVoid();
			}
		}

	}

	std::string error_str;
	llvm::raw_string_ostream error_stream(error_str);

	auto has_error = llvm::verifyFunction(*fn, &error_stream);

	if (has_error)
	{
		ctx.module->print(error_stream, nullptr);
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

llvm::Value * pars::IfStmt::emit(EmitCtx &ctx)
{
	auto *condition_value = condition->emit(ctx);

	auto *fn = ctx.builder.GetInsertBlock()->getParent();

	auto *then_bb = llvm::BasicBlock::Create(*ctx.llvm_ctx, "then", fn);

	llvm::BasicBlock *else_bb;

	if (else_br != nullptr)
	{
		else_bb = llvm::BasicBlock::Create(*ctx.llvm_ctx, "else", fn);
	}

	auto *merge_bb = llvm::BasicBlock::Create(*ctx.llvm_ctx, "merge", fn);

	if (else_br == nullptr)
	{
		else_bb = merge_bb;
	}

	ctx.builder.CreateCondBr(condition_value, then_bb, else_bb);

	ctx.builder.SetInsertPoint(then_bb);

	body->emit(ctx);

	ctx.builder.CreateBr(merge_bb);

	if (else_br != nullptr)
	{
		ctx.builder.SetInsertPoint(else_bb);
		else_br->emit(ctx);
		ctx.builder.CreateBr(merge_bb);
	}

	ctx.builder.SetInsertPoint(merge_bb);

	return merge_bb;
}

llvm::Value * pars::WhileStmt::emit(EmitCtx &ctx)
{

	auto *fn = ctx.builder.GetInsertBlock()->getParent();

	auto *before_bb = llvm::BasicBlock::Create(*ctx.llvm_ctx, "before", fn);

	ctx.builder.CreateBr(before_bb);

	ctx.builder.SetInsertPoint(before_bb);

	auto *condition_value = condition->emit(ctx);

	auto *then_bb = llvm::BasicBlock::Create(*ctx.llvm_ctx, "then", fn);
	auto *merge_bb = llvm::BasicBlock::Create(*ctx.llvm_ctx, "merge", fn);

	ctx.builder.CreateCondBr(condition_value, then_bb, merge_bb);

	ctx.builder.SetInsertPoint(then_bb);

	body->emit(ctx);

	ctx.builder.CreateBr(before_bb);

	ctx.builder.SetInsertPoint(merge_bb);

	return merge_bb;
}

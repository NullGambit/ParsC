#include "stmt.hpp"

#include <iostream>
#include <llvm/IR/Verifier.h>
#include "llvm/IR/ConstantFold.h"

#include "compile_error.hpp"
#include "expr.hpp"
#include "frontend_error.hpp"
#include "type.hpp"
#include "debug/ast_printer.hpp"
#include "magic_enum/magic_enum.hpp"
#include "util/fmt.hpp"

llvm::Value * pars::VarDeclStmt::emit(EmitCtx &ctx)
{
	return init(ctx);
}

llvm::Value * pars::VarDeclStmt::init(EmitCtx &ctx, llvm::Value *value)
{
	if (value == nullptr)
	{
		if (initializer == nullptr)
		{
			value = type->get_default_value(ctx.llvm_ctx);
		}
		else
		{
			value = initializer->emit(ctx);
		}
	}

	if (!has_flag(type_meta.flags, TypeFlags::Const))
	{
		auto *inst = ctx.builder.CreateAlloca(type->get_llvm_type(ctx.llvm_ctx), nullptr, symbol.name);

		ctx.builder.CreateStore(value, inst, has_flag(flags, VarFlags::Volatile));

		value = inst;
	}

	ctx.named_values[symbol.name] = value;

	return value;
}

bool pars::VarDeclStmt::is_explicitly_typed() const
{
	return !type_meta.name.empty();
}

bool pars::VarDeclStmt::is_type_inferred() const
{
	return !is_explicitly_typed();
}

llvm::Value* pars::AssignmentStmt::emit(EmitCtx &ctx)
{
	auto *value = ctx.named_values[symbol];

	if (value == nullptr || !value->getType()->isPointerTy())
	{
		throw CompileError{this, fmt::format("{} is not mutable", symbol)};
	}

	using enum TokenType;

	auto *rhs_value = rhs->emit(ctx);

	// TODO this can create a duplicate instruction pls fix
	auto *ptr_value = ctx.builder.CreateLoad(llvm::PointerType::get(*ctx.llvm_ctx, 0), value);

	if (op == Equal)
	{
		return ctx.builder.CreateStore(rhs_value, ptr_value);
	}

	auto *curr_value = lhs->emit(ctx);

	// TODO i commented this out cause it can sometimes carryover instructions from other functions
	// i didnt really think this through that well. whoops
	//auto *ptr_value = ctx.pointer_cache[symbol];

	TokenType op_op {};

	switch (op)
	{
		case PlusEqual: op_op = Plus; break;
		case MinusEqual: op_op = Minus; break;
		case StarEqual: op_op = Star; break;
		case SlashEqual: op_op = ForwardSlash; break;
		default: throw CompileError{this, fmt::format("operator {} is not allowed for op apply", magic_enum::enum_name(op))};
	}

	auto *new_value = lhs->type->op_binary(ctx, op_op, curr_value, rhs_value);

	if (new_value == nullptr)
	{
		throw CompileError{rhs, fmt::format("this operation is not defined for type of {}", lhs->type->get_type_name())};
	}

	if (dynamic_cast<Pointer*>(lhs->type))
	{
		return ctx.builder.CreateStore(new_value, value);
	}

	return ctx.builder.CreateStore(new_value, ptr_value);
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

	auto *ft = llvm::FunctionType::get(return_type->get_llvm_type(llvm_ctx), param_types, is_variadic);

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

	for (auto i = 0; auto *node : nodes)
	{
		// set insert point back to this functions basic block in case
		// there is a local function being emitted
		// ctx.builder.SetInsertPoint(bb);
		auto *value = node->emit(ctx);

		if (value == nullptr)
		{
			break;
		}

		if (auto *new_bb = llvm::dyn_cast_if_present<llvm::BasicBlock>(value))
		{
			bb = new_bb;
		}

		ctx.builder.SetInsertPoint(bb);

		auto *ret = dynamic_cast<TerminatorStmt*>(node);

		// eliminate dead code so llvm doesnt complain about terminator in the middle of basic block
		if (ret != nullptr)
		{
			break;
		}

		i++;
	}

	return bb;
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

		for (auto i = 0; auto &arg : fn->args())
		{
			auto *param = signature.parameters[i];

			param->init(ctx, &arg);

			i++;
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

	// TODO when im confident my code gen isnt dogshit anymore remove verification from release builds
	auto has_error = llvm::verifyFunction(*fn, &error_stream);

	if (has_error)
	{
		// print error to the bottom of the module ir
		// otherwise easy to miss verification errors
		std::string module_str;
		llvm::raw_string_ostream module_stream(module_str);

		ctx.module->print(module_stream, nullptr);

		module_str += error_str;

		error_stream.flush();
		fn->eraseFromParent();

		throw CompileError{this, std::move(module_str)};
	}

	return fn;
}

llvm::Value* pars::ReturnStmt::emit(EmitCtx &ctx)
{
	auto *value = expr->emit(ctx);

	return ctx.builder.CreateRet(value);
}

llvm::Value * pars::IfStmt::emit(EmitCtx &ctx)
{
	auto *condition_value = condition->emit(ctx);

	auto *start_bb = ctx.builder.GetInsertBlock();
	auto *fn = start_bb->getParent();

	auto has_return = [](llvm::BasicBlock *bb)
	{
		if (bb == nullptr)
		{
			return false;
		}

		auto *terminator = bb->getTerminator();

		return terminator != nullptr;
	};

	auto *then_bb = llvm::BasicBlock::Create(*ctx.llvm_ctx, "then", fn);

	ctx.builder.SetInsertPoint(then_bb);

	body->emit(ctx);

	llvm::BasicBlock *else_bb {};

	if (else_br != nullptr)
	{
		else_bb = llvm::BasicBlock::Create(*ctx.llvm_ctx, "else", fn);
		ctx.builder.SetInsertPoint(else_bb);
		else_br->emit(ctx);
	}

	if (has_return(else_bb) && has_return(then_bb))
	{
		ctx.builder.SetInsertPoint(start_bb);
		ctx.builder.CreateCondBr(condition_value, then_bb, else_bb);

		return nullptr;
	}

	auto *merge_bb = llvm::BasicBlock::Create(*ctx.llvm_ctx, "merge", fn);

	if (else_br == nullptr)
	{
		else_bb = merge_bb;
	}

	ctx.builder.SetInsertPoint(start_bb);
	ctx.builder.CreateCondBr(condition_value, then_bb, else_bb);

	auto do_maybe_br = [&ctx, &has_return](llvm::BasicBlock *this_bb, llvm::BasicBlock *next_bb)
	{
		if (!has_return(this_bb))
		{
			ctx.builder.SetInsertPoint(this_bb);
			ctx.builder.CreateBr(next_bb);
		}
	};

	do_maybe_br(then_bb, merge_bb);
	if (else_br != nullptr)
	{
		do_maybe_br(else_bb, merge_bb);
	}

	ctx.builder.SetInsertPoint(merge_bb);

	return merge_bb;
}

llvm::Value * pars::CompIfStmt::emit(EmitCtx &ctx)
{
	auto *value = stmt->condition->emit(ctx);

	// TODO could throw an error here but for now ill allow a silent fail to else
	if (auto *constant = llvm::dyn_cast<llvm::ConstantInt>(value); constant && constant->getValue() == true)
	{
		return stmt->body->emit(ctx);
	}
	if (stmt->else_br != nullptr)
	{
		return stmt->else_br->emit(ctx);
	}

	return nullptr;
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

	ctx.loop_bbs.emplace_back(merge_bb, before_bb);

	body->emit(ctx);

	ctx.builder.CreateBr(before_bb);

	ctx.builder.SetInsertPoint(merge_bb);

	ctx.loop_bbs.pop_back();

	return merge_bb;
}

llvm::Value* pars::ForStmt::emit(EmitCtx &ctx)
{
	std::vector<llvm::Value*> binding_values;

	binding_values.reserve(bindings.size());

	for (auto *binding : bindings)
	{
		binding_values.emplace_back(binding->emit(ctx));
	}

	llvm::Value *index_ptr = nullptr;

	if (has_index())
	{
		index_ptr = binding_values.back();
		binding_values.pop_back();
	}

	auto *fn = ctx.builder.GetInsertBlock()->getParent();

	auto *preloop_bb = llvm::BasicBlock::Create(*ctx.llvm_ctx, "preloop", fn);
	auto *body_bb = llvm::BasicBlock::Create(*ctx.llvm_ctx, "body", fn);
	auto *update_bb = llvm::BasicBlock::Create(*ctx.llvm_ctx, "update", fn);
	auto *merge_bb = llvm::BasicBlock::Create(*ctx.llvm_ctx, "merge", fn);

	ctx.loop_bbs.emplace_back(merge_bb, update_bb);

	iterable->type->iter_emit_init(ctx, iterable, binding_values);

	ctx.builder.CreateBr(preloop_bb);
	ctx.builder.SetInsertPoint(preloop_bb);

	auto *cond = iterable->type->iter_emit_condition(ctx, iterable, binding_values);

	ctx.builder.CreateCondBr(cond, body_bb, merge_bb);

	ctx.builder.SetInsertPoint(body_bb);

	body->emit(ctx);

	ctx.builder.CreateBr(update_bb);

	ctx.builder.SetInsertPoint(update_bb);

	iterable->type->iter_emit_update(ctx, iterable, binding_values);

	if (index_ptr != nullptr)
	{
		auto *index_type = bindings.back()->type->get_llvm_type(ctx.llvm_ctx);
		auto *v = ctx.builder.CreateLoad(index_type, index_ptr);
		auto *inc = I32Type.op_binary(ctx, TokenType::Plus, v, llvm::ConstantInt::get(*ctx.llvm_ctx, llvm::APInt(32, 1)));
		ctx.builder.CreateStore(inc, index_ptr);
	}

	ctx.builder.CreateBr(preloop_bb);

	ctx.loop_bbs.pop_back();

	return merge_bb;
}

bool pars::ForStmt::has_index() const
{
	return bindings.size() == iterable->type->get_iter_bindings().size() + 1;
}

llvm::Value* do_break_continue(pars::Node *node, pars::EmitCtx &ctx, llvm::BasicBlock *bb, std::string_view type)
{
	if (ctx.loop_bbs.empty())
	{
		throw pars::CompileError{node, fmt::format("cannot {} outside of a loop", type)};
	}

	return ctx.builder.CreateBr(bb);
}

llvm::Value * pars::BreakStmt::emit(EmitCtx &ctx)
{
	return do_break_continue(this, ctx, ctx.loop_bbs.back().merge, "break");
}

llvm::Value * pars::ContinueStmt::emit(EmitCtx &ctx)
{
	return do_break_continue(this, ctx, ctx.loop_bbs.back().start, "break");
}

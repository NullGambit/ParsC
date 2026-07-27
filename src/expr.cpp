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
#include "util/llvm_utils.hpp"

static pars::HashMap<std::string_view, llvm::GlobalVariable*> g_static_strings;

llvm::Value * pars::LiteralExpr::emit(EmitCtx &ctx, EmitParams params)
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
			auto value = llvm::APInt(1, _bool);
			return (llvm::Value*)llvm::ConstantInt::get(*ctx.llvm_ctx, value);
		},
		[&ctx](char _char)
		{
			auto value = llvm::APInt(8, _char);
			return (llvm::Value*)llvm::ConstantInt::get(*ctx.llvm_ctx, value);
		},
		[&ctx](std::nullptr_t)
		{
			return VoidPointerType.get_default_value(ctx.llvm_ctx);
		}
	}, value);
}

llvm::Value * pars::BinaryExpr::emit(EmitCtx &ctx, EmitParams params)
{
	llvm::Value *result {};

	if (left->type->is_array())
	{
		result = left->type->op_binary(ctx, op, left->emit_ptr(ctx), right->emit_ptr(ctx));
	}
	else
	{
		auto lhs = left->emit(ctx);
		auto rhs = right->emit(ctx);

		result = left->type->op_binary(ctx, op, lhs, rhs);
	}

	if (result == nullptr)
	{
		throw CompileError{this,
			fmt::format("Cannot use binary operator '{}' on operands {}", token.lexeme, type->get_type_name())};
	}

	return result;
}

llvm::Value * pars::UnaryExpr::emit(EmitCtx &ctx, EmitParams params)
{
	auto *rhs = right->emit(ctx);

	TokenType tk_type {};

	if (op == '!')
	{
		tk_type = TokenType::Bang;
	}
	if (op == '-')
	{
		tk_type = TokenType::Minus;
	}

	auto *result = type->op_unary(ctx, tk_type, rhs);

	if (result == nullptr)
	{
		throw CompileError{this,
			fmt::format("Cannot use unary operator '{}' on operands {}", token.lexeme, type->get_type_name())};
	}

	return result;
}

llvm::Value* pars::SymbolExpr::emit(EmitCtx &ctx, EmitParams params)
{
	auto *value = emit_ptr(ctx);

	if (value == nullptr)
	{
		return nullptr;
	}

	//if (llvm::isa<llvm::AllocaInst>(value))
	if (value->getType()->isPointerTy())
	{
		value = ctx.builder.CreateLoad(type->get_llvm_type(ctx.llvm_ctx), value, symbol);
	}

	return value;
}

llvm::Value * pars::SymbolExpr::emit_ptr(EmitCtx &ctx, EmitParams params)
{
	return ctx.named_values[symbol];
}

llvm::Value * pars::CallExpr::emit(EmitCtx &ctx, EmitParams params)
{
	auto *fn = ctx.module->getFunction(symbol);

	if (fn == nullptr)
	{
		fn = prototype->signature.emit(ctx, symbol, prototype->flags);
	}

	std::vector<llvm::Value*> argv;

	argv.reserve(fn->arg_size());

	u32 index {};

	for (auto *arg : arguments)
	{
		Type *desired_type {};

		if (index < prototype->signature.parameters.size())
		{
			desired_type = prototype->signature.parameters[index]->type;

			if (!is_assignable_from(arg->type, desired_type))
			{
				throw CompileError
				{
					this,
					fmt::format
					(
						"expected type {} instead of {} at position {}",
						desired_type->get_type_name(),
						arg->type->get_type_name(),
						index
					)
				};
			}
		}

		llvm::Value *value {};

		if (desired_type != nullptr && arg->type->can_coerce_into(desired_type))
		{
			value = arg->type->op_coerce(ctx, arg->emit_ptr(ctx), desired_type);
		}
		else
		{
			value = arg->emit(ctx);
		}

		argv.emplace_back(value);

		index++;
	}

	// default params
	for (auto i = index; i < prototype->signature.parameters.size(); i++)
	{
		auto *param = prototype->signature.parameters[index];
		auto *value = ctx.builder.CreateLoad(param->type->get_llvm_type(ctx.llvm_ctx), param->emit(ctx));

		argv.emplace_back(value);
	}

	return ctx.builder.CreateCall(fn, argv);
}

llvm::Value * pars::GroupExpr::emit(EmitCtx &ctx, EmitParams params)
{
	return inner->emit(ctx);
}

llvm::Value * pars::GroupExpr::emit_ptr(EmitCtx &ctx, EmitParams params)
{
	return inner->emit_ptr(ctx);
}

llvm::Value * pars::SizeofExpr::emit(EmitCtx &ctx, EmitParams params)
{
	auto value = llvm::APInt(32, expr->type->get_size());
	return llvm::ConstantInt::get(*ctx.llvm_ctx, value);
}

llvm::Value* pars::MemberAccessExpr::emit(EmitCtx& ctx, EmitParams params)
{
	auto *ptr = emit_ptr(ctx);

	if (ptr != nullptr && ptr->getType()->isPointerTy())
	{
		return ctx.builder.CreateLoad(type->get_llvm_type(ctx.llvm_ctx), ptr);
	}

	return ptr;
}

llvm::Value * pars::MemberAccessExpr::emit_ptr(EmitCtx &ctx, EmitParams params)
{
	auto *target_value = target->emit_ptr(ctx);
	auto *accessor_value = accessor->emit(ctx);

	if (target_value == nullptr)
	{
		target_value = target->emit(ctx);
	}

	// case: import_alias.func()
	if (target_value == nullptr)
	{
		return accessor_value;
	}

	if (target->type->is_ptr())
	{
		target_value = ctx.builder.CreateLoad(target->type->get_llvm_type(ctx.llvm_ctx), target_value);
	}

	auto *result = target->type->access_member(ctx, target_value, accessor_value, accessor->get_symbol());

	if (result == nullptr)
	{
		throw CompileError{this, fmt::format("property '{}' does not exist for type {}", accessor->get_symbol(), target->type->get_type_name())};
	}

	auto member = target->type->get_member(accessor->get_symbol()).value();

	if (member.access == MemberAccess::Readonly || member.access == MemberAccess::Private)
	{
		auto *node = llvm::MDNode::get(*ctx.llvm_ctx, {});

		set_metadata(ctx.llvm_ctx, result, node, "Const");
	}

	return result;
}

std::string_view pars::MemberAccessExpr::get_symbol()
{
	return target->get_symbol();
}

llvm::Value* pars::TypePropExpr::emit(EmitCtx& ctx, EmitParams params)
{
	auto *prop = type->get_property(ctx.llvm_ctx, property_name);

	if (prop == nullptr)
	{
		throw CompileError{this, fmt::format("property '{}' does not exist for type {}", property_name, type->get_type_name())};
	}

	return prop;
}

llvm::Value* pars::CastExpr::emit(EmitCtx& ctx, EmitParams params)
{
	auto *value = target->emit(ctx);

	auto *result = original_type->op_cast(ctx, value, type);

	if (result == nullptr)
	{
		throw CompileError{this, fmt::format("cannot cast {} to {}",
			original_type->get_type_name(), type->get_type_name())};
	}

	return result;
}

llvm::Value * pars::NamedExpr::emit(EmitCtx &ctx, EmitParams params)
{
	return value->emit(ctx);
}

llvm::Value * emit_brace_list(pars::EmitCtx &ctx, pars::EmitParams params, const pars::Type *type, const pars::BraceInitList &initializers)
{
	auto *llvm_type = type->get_llvm_type(ctx.llvm_ctx);

	auto should_return = false;

	if (params.target_ptr == nullptr)
	{
		params.target_ptr = get_alloca_builder(ctx).CreateAlloca(llvm_type);
		should_return = true;
	}

	ctx.builder.CreateStore(type->get_default_value(ctx.llvm_ctx), params.target_ptr);

	for (auto i = 0; auto [initializer, pos] : initializers)
	{
		auto *field = ctx.builder.CreateGEP(llvm_type, params.target_ptr,
			{ctx.builder.getInt32(0), ctx.builder.getInt32(pos)});

		auto *result = initializer->emit(ctx, {.target_ptr = field});

		if (result != nullptr)
		{
			ctx.builder.CreateStore(result, field);
		}

		i++;
	}

	return should_return ? ctx.builder.CreateLoad(llvm_type, params.target_ptr) : nullptr;
}

llvm::Value * pars::AnonInitExpr::emit(EmitCtx &ctx, EmitParams params)
{
	if (values.empty())
	{
		return type->get_default_value(ctx.llvm_ctx);
	}

	return emit_brace_list(ctx, params, type, values);
}

llvm::Value * pars::AbsExpr::emit(EmitCtx &ctx, EmitParams params)
{
	auto *llvm_value = value->emit(ctx);

	auto *result = value->type->op_abs(ctx, llvm_value);

	if (result == nullptr)
	{
		throw CompileError{this, fmt::format("abs is not defined for type {}", value->type->get_type_name())};
	}

	return result;
}

llvm::Value* pars::PtrOpExpr::emit(EmitCtx &ctx, EmitParams params)
{
	using enum TokenType;

	auto *value = emit_ptr(ctx);

	switch (op)
	{
		case Ampersand:
		{
			return value;
		}
		case Caret:
		{
			auto *inner = dynamic_cast<Pointer*>(target->type)->inner;

			if (inner->is_equal(&VoidType))
			{
				throw CompileError{this, "Cannot dereference void pointer. size is not known"};
			}

			return ctx.builder.CreateLoad(inner->get_llvm_type(ctx.llvm_ctx), value);
		}
		default: return nullptr;
 	}
}

llvm::Value * pars::PtrOpExpr::emit_ptr(EmitCtx &ctx, EmitParams params)
{
	using enum TokenType;

	auto *value = target->emit_ptr(ctx);

	if (!value->getType()->isPointerTy())
	{
		throw CompileError{this, "target is not a pointer"};
	}

	switch (op)
	{
		case Ampersand:
		{
			return value;
		}
		case Caret:
		{
			return ctx.builder.CreateLoad(llvm::PointerType::get(*ctx.llvm_ctx, 0), value);
		}
		default: return nullptr;
	}
}

llvm::Value * pars::PackedExpr::emit(EmitCtx &ctx, EmitParams params)
{
	return Expr::emit(ctx);
}

llvm::Value * pars::ArrayLiteralExpr::emit(EmitCtx &ctx, EmitParams params)
{
	auto *array_type = type->get_llvm_type(ctx.llvm_ctx);

	auto should_return = false;

	if (params.target_ptr == nullptr)
	{
		params.target_ptr = get_alloca_builder(ctx).CreateAlloca(array_type);
		should_return = true;
	}

	for (auto i = 0; auto *element : elements)
	{
		auto *element_value = element->emit(ctx);

		auto *ptr = ctx.builder.CreateInBoundsGEP(array_type, params.target_ptr,
			{ctx.builder.getInt64(0), ctx.builder.getInt64(i)});

		ctx.builder.CreateStore(element_value, ptr);

		i++;
	}

	return should_return ? ctx.builder.CreateLoad(array_type, params.target_ptr) : nullptr;
}

llvm::Value * pars::IndexOpExpr::emit(EmitCtx &ctx, EmitParams params)
{
	auto *result = lhs->type->op_index(ctx, lhs->emit_ptr(ctx), index->emit(ctx));

	if (result == nullptr)
	{
		throw CompileError{this, fmt::format("type of {} cannot be indexed", type->get_type_name())};
	}

	return result;
}

llvm::Value * pars::IndexOpExpr::emit_ptr(EmitCtx &ctx, EmitParams params)
{
	auto *array = lhs->emit_ptr(ctx);
	auto *index_value = index->emit(ctx);

	return ctx.builder.CreateInBoundsGEP(lhs->type->get_llvm_type(ctx.llvm_ctx), array,
		{ctx.builder.getInt64(0), index_value});
}

llvm::Value * pars::StructLiteral::emit(EmitCtx &ctx, EmitParams params)
{
	return emit_brace_list(ctx, params, type, initializers);
}

llvm::Value * pars::SliceExpr::emit(EmitCtx &ctx, EmitParams params)
{
	emit_ptr(ctx, params);

	return params.target_ptr == nullptr ? m_cached_result : nullptr;
}

llvm::Value * pars::SliceExpr::emit_ptr(EmitCtx &ctx, EmitParams params)
{
	auto *ptr = lhs->emit_ptr(ctx);

	if (params.target_ptr == nullptr)
	{
		params.target_ptr = get_alloca_builder(ctx).CreateAlloca(type->get_llvm_type(ctx.llvm_ctx));
	}

	llvm::Value *start_value {};
	llvm::Value *end_value {};

	if (start != nullptr)
	{
		start_value = start->emit(ctx, params);
	}
	else
	{
		start_value = ctx.builder.getInt32(0);
	}

	if (end != nullptr)
	{
		end_value = end->emit(ctx, params);
	}
	else
	{
		auto *array_type = dynamic_cast<Array*>(lhs->type);
		end_value = ctx.builder.getInt32(array_type->size);
	}

	m_cached_result = lhs->type->op_slice(ctx, ptr, params.target_ptr, start_value, end_value);

	if (m_cached_result == nullptr)
	{
		throw CompileError{this, fmt::format("cannot slice type {}", lhs->type->get_type_name())};
	}

	return params.target_ptr;
}

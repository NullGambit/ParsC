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

llvm::Value * pars::BinaryExpr::emit(EmitCtx &ctx)
{
	auto lhs = left->emit(ctx);
	auto rhs = right->emit(ctx);

	auto *result = left->type->op_binary(ctx, op, lhs, rhs);

	if (result == nullptr)
	{
		throw CompileError{this,
			fmt::format("Cannot use binary operator '{}' on operands {}", token.lexeme, type->get_type_name())};
	}

	return result;
}

llvm::Value * pars::UnaryExpr::emit(EmitCtx &ctx)
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

llvm::Value* pars::SymbolExpr::emit(EmitCtx &ctx)
{
	if (type == nullptr)
	{
		throw CompileError{this, fmt::format("unknown symbol '{}'", symbol)};
	}

	auto *value = emit_ptr(ctx);

	if (llvm::isa<llvm::AllocaInst>(value))
	{
		value = ctx.builder.CreateLoad(type->get_llvm_type(ctx.llvm_ctx), value, symbol);
	}

	return value;
}

llvm::Value * pars::SymbolExpr::emit_ptr(EmitCtx &ctx)
{
	return ctx.named_values[symbol];
}

llvm::Value * pars::CallExpr::emit(EmitCtx &ctx)
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

		auto *value = arg->emit(ctx);

		if (desired_type != nullptr && desired_type->can_coerce_into(arg->type))
		{
			value = arg->type->op_coerce(ctx, value, desired_type);
		}

		argv.emplace_back(value);

		index++;
	}

	// default params
	for (auto i = index; i < prototype->signature.parameters.size(); i++)
	{
		auto *param = prototype->signature.parameters[index];

		argv.emplace_back(param->emit(ctx));
	}

	return ctx.builder.CreateCall(fn, argv);
}

llvm::Value * pars::GroupExpr::emit(EmitCtx &ctx)
{
	return inner->emit(ctx);
}

llvm::Value * pars::GroupExpr::emit_ptr(EmitCtx &ctx)
{
	return inner->emit_ptr(ctx);
}

llvm::Value * pars::SizeofExpr::emit(EmitCtx &ctx)
{
	auto value = llvm::APInt(32, expr->type->get_size());
	return llvm::ConstantInt::get(*ctx.llvm_ctx, value);
}

llvm::Value* pars::MemberAccessExpr::emit(EmitCtx& ctx)
{
	return accessor->emit(ctx);
}

llvm::Value* pars::TypePropExpr::emit(EmitCtx& ctx)
{
	auto *prop = type->get_property(ctx.llvm_ctx, property_name);

	if (prop == nullptr)
	{
		throw CompileError{this, fmt::format("property '{}' does not exist for type {}", property_name, type->get_type_name())};
	}

	return prop;
}

llvm::Value* pars::CastExpr::emit(EmitCtx& ctx)
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

llvm::Value * pars::NamedExpr::emit(EmitCtx &ctx)
{
	return value->emit(ctx);
}

llvm::Value * pars::AnonInitExpr::emit(EmitCtx &ctx)
{
	if (values.empty())
	{
		return type->get_default_value(ctx.llvm_ctx);
	}
}

llvm::Value * pars::AbsExpr::emit(EmitCtx &ctx)
{
	auto *llvm_value = value->emit(ctx);

	auto *result = value->type->op_abs(ctx, llvm_value);

	if (result == nullptr)
	{
		throw CompileError{this, fmt::format("abs is not defined for type {}", value->type->get_type_name())};
	}

	return result;
}

llvm::Value* pars::PtrOpExpr::emit(EmitCtx &ctx)
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

llvm::Value * pars::PtrOpExpr::emit_ptr(EmitCtx &ctx)
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

llvm::Value * pars::PackedExpr::emit(EmitCtx &ctx)
{
	return Expr::emit(ctx);
}

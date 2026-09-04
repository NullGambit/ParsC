#include "type.hpp"

#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Type.h>

#include <cmath>
#include <numeric>
#include <llvm/IR/Verifier.h>

#include "compile_error.hpp"
#include "emit_context.hpp"
#include "expr.hpp"
#include "stmt.hpp"
#include "util/fmt.hpp"
#include "util/llvm_utils.hpp"

llvm::Value * pars::Type::get_property(llvm::LLVMContext *ctx, std::string_view name)
{
	if (name == "init")
	{
		return get_default_value(ctx);
	}

	return nullptr;
}

bool pars::check_type_equality(Type const *a_type, Type const *b_type)
{
	return a_type->is_equal(b_type) || b_type->is_equal(a_type);
}

bool pars::is_assignable_from(Type const *from, Type const *to)
{
	return check_type_equality(from, to) || to->can_coerce_into(from) || from->can_coerce_into(to);
}

llvm::Type * pars::Void::get_llvm_type(llvm::LLVMContext *ctx) const
{
	return llvm::Type::getVoidTy(*ctx);
}

llvm::Value * pars::Void::get_default_value(llvm::LLVMContext *ctx) const
{
	return llvm::UndefValue::get(get_llvm_type(ctx));
}

llvm::Type * pars::Integral::get_llvm_type(llvm::LLVMContext *ctx) const
{
	return llvm::IntegerType::get(*ctx, bits);
}

llvm::Value * pars::Integral::get_default_value(llvm::LLVMContext *ctx) const
{
	return llvm::ConstantInt::get(*ctx, llvm::APInt(bits, 0, is_signed));
}

llvm::Value* pars::Integral::get_property(llvm::LLVMContext* ctx, std::string_view name)
{
	if (auto *value = Type::get_property(ctx, name))
	{
		return value;
	}

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

llvm::Value * pars::Integral::op_cast(EmitCtx &ctx, llvm::Value *value, Type *desired_type) const
{
	auto *target_type = desired_type->get_llvm_type(ctx.llvm_ctx);

	auto *other_type = dynamic_cast<Integral*>(desired_type);

	if (other_type == nullptr)
	{
		return nullptr;
	}

	const auto op = llvm::CastInst::getCastOpcode(value, is_signed, target_type, other_type->is_signed);

	return ctx.builder.CreateCast(op, value, target_type);
}

llvm::Value * pars::Integral::op_coerce(EmitCtx &ctx, llvm::Value *value, Type *desired_type) const
{
	return op_cast(ctx, value, desired_type);
}

bool pars::Integral::can_coerce_into(Type const *desired_type) const
{
	auto *other_type = dynamic_cast<const Integral*>(desired_type);

	return bits > other_type->bits || (bits == other_type->bits && is_signed == other_type->is_signed);
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

	return type->is_equal(other);
}

llvm::Value * pars::AliasType::op_abs(EmitCtx &ctx, llvm::Value *value) const
{
	return type->op_abs(ctx, value);
}

llvm::Value * pars::AliasType::op_binary(EmitCtx &ctx, TokenType op, llvm::Value *lhs, llvm::Value *rhs) const
{
	return type->op_binary(ctx, op, lhs, rhs);
}

llvm::Value * pars::AliasType::op_cast(EmitCtx &ctx, llvm::Value *value, Type *desired_type) const
{
	return type->op_cast(ctx, value, desired_type);
}

llvm::Value * pars::AliasType::op_coerce(EmitCtx &ctx, llvm::Value *value, Type *desired_type) const
{
	return type->op_coerce(ctx, value, desired_type);
}

llvm::Value * pars::AliasType::op_in(EmitCtx &ctx, llvm::Value *lhs, llvm::Value *rhs) const
{
	return type->op_in(ctx, lhs, rhs);
}

llvm::Value * pars::AliasType::op_unary(EmitCtx &ctx, TokenType op, llvm::Value *rhs) const
{
	return type->op_unary(ctx, op, rhs);
}

llvm::Value * pars::AliasType::op_index(EmitCtx &ctx, llvm::Value *target, llvm::Value *index) const
{
	return type->op_index(ctx, target, index);
}

llvm::Value * pars::AliasType::op_call(EmitCtx &ctx, llvm::Value *callable, llvm::ArrayRef<llvm::Value *> args) const
{
	return type->op_call(ctx, callable, args);
}

u32 pars::AliasType::get_size()
{
	return type->get_size();
}

llvm::Value * pars::AliasType::get_default_value(llvm::LLVMContext *ctx) const
{
	return type->get_default_value(ctx);
}

std::optional<pars::CallInfo> pars::AliasType::get_call_info()
{
	return type->get_call_info();
}

bool pars::AliasType::is_ptr() const
{
	return type->is_ptr();
}

bool pars::AliasType::is_array() const
{
	return type->is_array(); //|| has_flag(meta.flags, TypeFlags::Array);
}

bool pars::AliasType::is_struct() const
{
	return type->is_struct();
}

pars::Type * pars::AliasType::get_inner() const
{
	return type->get_inner();
}

bool pars::AliasType::is_callable() const
{
	return type->is_callable();
}

std::optional<pars::MemberInfo> pars::AliasType::get_member(std::string_view symbol) const
{
	return type->get_member(symbol);
}

llvm::Value * pars::AliasType::access_member(EmitCtx &ctx, llvm::Value *ptr, llvm::Value *accessor,
	std::string_view symbol) const
{
	return type->access_member(ctx, ptr, accessor, symbol);
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
		case EqualEqual: return ctx.builder.CreateICmpEQ(lhs, rhs);
		case BangEqual: return ctx.builder.CreateICmpNE(lhs, rhs);
		case GreaterEqual: return ctx.builder.CreateICmpSGT(lhs, rhs);
		case LessEqual: return ctx.builder.CreateICmpSLE(lhs, rhs);
		case Less:
		{
			if (is_signed)
			{
				return ctx.builder.CreateICmpSLT(lhs, rhs);
			}

			return ctx.builder.CreateICmpULT(lhs, rhs);
		}
		case Greater: return ctx.builder.CreateICmpSGT(lhs, rhs);
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

llvm::Value * pars::Integer::op_abs(EmitCtx &ctx, llvm::Value *value) const
{
	auto *is_poison = ctx.builder.getInt1(true);

	return ctx.builder.CreateIntrinsic(llvm::Intrinsic::abs, get_llvm_type(ctx.llvm_ctx), {value, is_poison});
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
		case StarStar: return ctx.builder.CreateBinaryIntrinsic(llvm::Intrinsic::pow, lhs, rhs);
		case EqualEqual: return ctx.builder.CreateFCmpOEQ(lhs, rhs);
		case BangEqual: return ctx.builder.CreateFCmpONE(lhs, rhs);
		case GreaterEqual: return ctx.builder.CreateFCmpOGE(lhs, rhs);
		case LessEqual: return ctx.builder.CreateFCmpOLE(lhs, rhs);
		case Less: return ctx.builder.CreateFCmpOLT(lhs, rhs);
		case Greater: return ctx.builder.CreateFCmpOGT(lhs, rhs);
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

llvm::Value * pars::Float::get_default_value(llvm::LLVMContext *ctx) const
{
	return llvm::ConstantFP::get(get_llvm_type(ctx), 0.0f);
}

llvm::Value * pars::Float::op_abs(EmitCtx &ctx, llvm::Value *value) const
{
	return ctx.builder.CreateIntrinsic(llvm::Intrinsic::fabs, get_llvm_type(ctx.llvm_ctx), {value});
}

llvm::Type * pars::Bool::get_llvm_type(llvm::LLVMContext *ctx) const
{
	return Integral::get_llvm_type(ctx);
}

llvm::Value * pars::Bool::op_binary(EmitCtx &ctx, TokenType op, llvm::Value *lhs, llvm::Value *rhs) const
{
	using enum TokenType;

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

llvm::Value * pars::Char::op_binary(EmitCtx &ctx, TokenType op, llvm::Value *lhs, llvm::Value *rhs) const
{
	return Integer{bits, is_signed, "char"}.op_binary(ctx, op, lhs, rhs);
}

llvm::Type * pars::Char::get_llvm_type(llvm::LLVMContext *ctx) const
{
	return Integral::get_llvm_type(ctx);
}

llvm::Type * pars::Pointer::get_llvm_type(llvm::LLVMContext *ctx) const
{
	return llvm::PointerType::get(inner->get_llvm_type(ctx), 0);
}

std::string_view pars::Pointer::get_type_name() const
{
	// TODO improve name to include inner type name as well
	return "pointer";
}

std::optional<pars::MemberInfo> pars::Pointer::get_member(std::string_view symbol) const
{
	return inner->get_member(symbol);
}

llvm::Value * pars::Pointer::access_member(EmitCtx &ctx, llvm::Value *ptr, llvm::Value *accessor,
	std::string_view symbol) const
{
	return inner->access_member(ctx, ptr, accessor, symbol);
}

llvm::Value * pars::Pointer::get_default_value(llvm::LLVMContext *ctx) const
{
	auto *type = static_cast<llvm::PointerType*>(get_llvm_type(ctx));
	return llvm::ConstantPointerNull::get(type);
}

llvm::Value * pars::Pointer::op_binary(EmitCtx &ctx, TokenType op, llvm::Value *lhs, llvm::Value *rhs) const
{
	using enum TokenType;

	switch (op)
	{
		case Plus: return ctx.builder.CreateGEP(inner->get_llvm_type(ctx.llvm_ctx), lhs, rhs);
		case Minus: return ctx.builder.CreateGEP(inner->get_llvm_type(ctx.llvm_ctx), lhs, ctx.builder.CreateNeg(rhs));
	}
	return Integer::op_binary(ctx, op, lhs, rhs);
}

bool pars::Pointer::is_equal(Type const *other) const
{
	auto *other_ptr = dynamic_cast<const Pointer*>(other);

	return
	other_ptr != nullptr
	&&
	(inner->is_equal(other_ptr->inner)
	||
	other_ptr->inner->is_equal(&VoidType) || inner->is_equal(&VoidType));
}

std::string_view pars::Packed::get_type_name() const
{
	return "packed";
}

llvm::Value * pars::Packed::get_default_value(llvm::LLVMContext *ctx) const
{
	return VoidType.get_default_value(ctx);
}

llvm::Value * pars::BaseArray::get_default_value(llvm::LLVMContext *ctx) const
{
	return llvm::ConstantAggregateZero::get(get_llvm_type(ctx));
}

llvm::Value * pars::BaseArray::op_index(EmitCtx &ctx, llvm::Value *target, llvm::Value *index) const
{
	auto *ptr = ctx.builder.CreateInBoundsGEP(get_llvm_type(ctx.llvm_ctx), target,
		{ctx.builder.getInt32(0), index}, "array.op_index");

	return ctx.builder.CreateLoad(element_type->get_llvm_type(ctx.llvm_ctx), ptr);
}

std::optional<pars::MemberInfo> pars::BaseArray::get_member(std::string_view symbol) const
{
	if (symbol == "length")
	{
		return MemberInfo{"length", const_cast<Integer*>(&U32Type), MemberAccess::Readonly};
	}
	if (symbol == "ptr")
	{
		auto *ptr = new_node<Pointer>();

		ptr->inner = element_type;

		return MemberInfo{"ptr", ptr, MemberAccess::Readonly};
	}

	return {};
}

pars::Type * pars::BaseArray::get_inner() const
{
	return element_type;
}

std::span<pars::Type *> pars::BaseArray::get_iter_bindings() const
{
	return std::span{const_cast<Type**>(&element_type), 1};
}

llvm::Value * pars::BaseArray::iter_emit_init(EmitCtx &ctx, Expr *iterable, std::span<llvm::Value *> vars) const
{
	iter_emit_update(ctx, iterable, vars);

	return nullptr;
}

llvm::Value * pars::BaseArray::iter_emit_condition(EmitCtx &ctx, Expr *iterable, std::span<llvm::Value *> vars) const
{
	using enum TokenType;

	auto *index = ctx.builder.CreateLoad(U32Type.get_llvm_type(ctx.llvm_ctx), vars[1]);
	auto *size = access_member(ctx, iterable->emit_ptr(ctx), nullptr, "length");

	return U32Type.op_binary(ctx, Less, index, size);
}

llvm::Value * pars::BaseArray::iter_emit_update(EmitCtx &ctx, Expr *iterable, std::span<llvm::Value *> vars) const
{
	auto *index = ctx.builder.CreateLoad(U32Type.get_llvm_type(ctx.llvm_ctx), vars[1]);

	auto *value = op_index(ctx, iterable->emit_ptr(ctx), index);

	return ctx.builder.CreateStore(value, vars[0]);
}

u32 pars::Array::get_size()
{
	return size * element_type->get_size();
}

llvm::Type * pars::Array::get_llvm_type(llvm::LLVMContext *ctx) const
{
	return llvm::ArrayType::get(element_type->get_llvm_type(ctx), size);
}

llvm::Constant* pars::Array::get_aggregate_constant(EmitCtx &ctx, llvm::ArrayRef<llvm::Constant *> init_list) const
{
	return llvm::ConstantArray::get((llvm::ArrayType*)get_llvm_type(ctx.llvm_ctx), init_list);
}

std::string_view pars::Array::get_type_name() const
{
	return "array";
}

bool pars::Array::is_equal(Type const *other) const
{
	auto *other_array = dynamic_cast<Array const*>(other);

	return other_array != nullptr
	&& (other_array->size == size || other_array->size == UNSIZED_ARRAY) && other_array->element_type->is_equal(element_type);
}

llvm::Value * pars::Array::op_binary(EmitCtx &ctx, TokenType op, llvm::Value *lhs, llvm::Value *rhs) const
{
	using enum TokenType;

	if (op != Plus && op != Minus && op != Star && op != ForwardSlash)
	{
		return nullptr;
	}

	const auto ALIGNMENT = llvm::Align(16);

	auto *vec_type = llvm::FixedVectorType::get(element_type->get_llvm_type(ctx.llvm_ctx), size);

	auto *aalloca = llvm::dyn_cast<llvm::AllocaInst>(lhs);
	auto *balloca = llvm::dyn_cast<llvm::AllocaInst>(rhs);

	if (aalloca != nullptr && balloca != nullptr)
	{
		aalloca->setAlignment(ALIGNMENT);
		balloca->setAlignment(ALIGNMENT);
	}

	auto *a = ctx.builder.CreateAlignedLoad(vec_type, lhs, ALIGNMENT);
	auto *b = ctx.builder.CreateAlignedLoad(vec_type, rhs, ALIGNMENT);

	//TODO maybe align back down. idk if it causes problems as of now. keeping this here in case i need to do that in the future
    // auto *result = ctx.builder.CreateAdd(a, b);
    //
    // auto *temp = ctx.builder.CreateAlloca(get_llvm_type(ctx.llvm_ctx));
    //
    // temp->setAlignment(llvm::Align(16));
    //
    // ctx.builder.CreateAlignedStore(result, temp, llvm::Align(16));
    //
    // return ctx.builder.CreateLoad(get_llvm_type(ctx.llvm_ctx), temp);

	return element_type->op_binary(ctx, op, a, b);
}

llvm::Value * pars::Array::access_member(EmitCtx &ctx, llvm::Value *target, llvm::Value *accessor,
	std::string_view symbol) const
{
	if (symbol == "length")
	{
		return ctx.builder.getInt(llvm::APInt(32, size));
	}
	if (symbol == "ptr")
	{
		return target;
	}

	auto index = get_member_index(symbol);

	return op_index(ctx, target, ctx.builder.getInt32(index));
}

std::optional<pars::MemberInfo> pars::Array::get_member(std::string_view symbol) const
{
	auto maybe_member = BaseArray::get_member(symbol);

	if (maybe_member.has_value())
	{
		return maybe_member;
	}

	auto index = get_member_index(symbol);

	if (index == -1)
	{
		return std::nullopt;
	}

	return MemberInfo{symbol, element_type};
}

bool pars::Array::can_coerce_into(Type const *desired_type) const
{
	return dynamic_cast<const Slice*>(desired_type);
}

llvm::StructType* get_slice_struct(llvm::LLVMContext *ctx)
{
	auto *ptr_type = llvm::PointerType::get(*ctx, 0);
	return llvm::StructType::get(*ctx, {ptr_type, pars::U32Type.get_llvm_type(ctx)});
}

llvm::Value * pars::Array::op_coerce(EmitCtx &ctx, llvm::Value *value, Type *desired_type) const
{
	auto *ptr = ctx.builder.CreateAlloca(get_slice_struct(ctx.llvm_ctx));

	return op_slice(ctx, value, ptr, ctx.builder.getInt64(0), ctx.builder.getInt32(size));
}

llvm::Value * pars::Array::op_slice(EmitCtx &ctx, llvm::Value *array, llvm::Value *target, llvm::Value *start,
	llvm::Value *end) const
{
	auto *slice_struct = get_slice_struct(ctx.llvm_ctx);

	auto *offset_ptr = ctx.builder.CreateInBoundsGEP(get_llvm_type(ctx.llvm_ctx), array,
			{ctx.builder.getInt64(0), start}, "slice_offsets");

	auto *array_ptr = ctx.builder.CreateConstInBoundsGEP2_32(slice_struct, target, 0, 0);
	auto *len_ptr = ctx.builder.CreateConstInBoundsGEP2_32(slice_struct, target, 0, 1);

	auto *length_value = ctx.builder.CreateSub(end, start);

	ctx.builder.CreateStore(length_value, len_ptr);

	ctx.builder.CreateStore(offset_ptr, array_ptr);

	return ctx.builder.CreateLoad(slice_struct, target);
}

int pars::Array::get_member_index(std::string_view member) const
{
	auto iter = std::ranges::find_if(members, [member](std::string_view name)
	{
		return name == member;
	});

	if (iter == members.end())
	{
		return -1;
	}

	auto index = std::distance(members.begin(), iter);

	if (index >= size)
	{
		return -1;
	}

	return index;
}

llvm::Type * pars::Slice::get_llvm_type(llvm::LLVMContext *ctx) const
{
	return get_slice_struct(ctx);
}

std::string_view pars::Slice::get_type_name() const
{
	return "slice";
}

bool pars::Slice::is_equal(Type const *other) const
{
	return other->is_array() && element_type->is_equal(other->get_inner());
}

llvm::Value * pars::Slice::op_index(EmitCtx &ctx, llvm::Value *target, llvm::Value *index) const
{
	auto *base_ptr = ctx.builder.CreateConstInBoundsGEP2_32(get_llvm_type(ctx.llvm_ctx), target, 0, 0, "slice.ptr");
	auto *ptr_type = llvm::PointerType::get(element_type->get_llvm_type(ctx.llvm_ctx), 0);
	auto *array = ctx.builder.CreateLoad(ptr_type, base_ptr);

	auto *ptr = ctx.builder.CreateGEP(element_type->get_llvm_type(ctx.llvm_ctx), array, {index}, "array.op_index");

	return ctx.builder.CreateLoad(element_type->get_llvm_type(ctx.llvm_ctx), ptr);
}

llvm::Value * pars::Slice::access_member(EmitCtx &ctx, llvm::Value *ptr, llvm::Value *accessor,
	std::string_view symbol) const
{
	if (symbol == "length")
	{
		auto *len_ptr = ctx.builder.CreateConstInBoundsGEP2_32(get_llvm_type(ctx.llvm_ctx), ptr, 0, 1);

		return ctx.builder.CreateLoad(U32Type.get_llvm_type(ctx.llvm_ctx), len_ptr, "slice.length");
	}
	if (symbol == "ptr")
	{
		return ctx.builder.CreateConstInBoundsGEP2_32(get_llvm_type(ctx.llvm_ctx), ptr, 0, 0, "slice.ptr");
	}

	return nullptr;
}

u32 pars::Struct::get_size()
{
	// TODO calculate alignment
	return std::accumulate(fields.begin(), fields.end(), u32{0}, [](u32 a, const StructField &b)
	{
		return a + b.type->get_size();
	});
}

llvm::Type * pars::Struct::get_llvm_type(llvm::LLVMContext *ctx) const
{
	auto *type = llvm::StructType::getTypeByName(*ctx, symbol.name);

	if (type == nullptr)
	{
		type = llvm::StructType::create(*ctx, symbol.name);

		std::vector<llvm::Type*> llvm_types;

		llvm_types.reserve(fields.size());

		for (auto &field : fields)
		{
			llvm_types.emplace_back(field.type->get_llvm_type(ctx));
		}

		type->setBody(llvm_types);
	}

	return type;
}

llvm::Constant* pars::Struct::get_aggregate_constant(EmitCtx &ctx, llvm::ArrayRef<llvm::Constant *> init_list) const
{
	return llvm::ConstantStruct::get((llvm::StructType*)get_llvm_type(ctx.llvm_ctx), init_list);
}

std::string_view pars::Struct::get_type_name() const
{
	return symbol.name;
}

llvm::Value * pars::Struct::get_default_value(llvm::LLVMContext *ctx) const
{
	return llvm::ConstantAggregateZero::get(get_llvm_type(ctx));
}

bool pars::Struct::is_equal(Type const *other) const
{
	auto *other_struct = dynamic_cast<Struct const*>(other);

	// do structural equality matching of either one is anon
	if (other_struct && symbol.name.empty() || other_struct->symbol.name.empty())
	{
		auto len = std::min(fields.size(), other_struct->fields.size());

		for (auto i = 0; i < len; i++)
		{
			auto &this_field = fields[i];
			auto &other_field = other_struct->fields[i];

			if (this_field.symbol.name != other_struct->symbol.name && !this_field.type->is_equal(other_field.type))
			{
				return false;
			}
		}

		return true;
	}

	return this == other;
}

llvm::Value * pars::Struct::access_member(EmitCtx &ctx, llvm::Value *ptr, llvm::Value *accessor,
	std::string_view symbol) const
{
	auto index = UINT32_MAX;

	for (auto i = 0; auto &field : fields)
	{
		if (field.symbol.name == symbol)
		{
			index = i;
			break;
		}

		i++;
	}

	if (index == UINT32_MAX)
	{
		return nullptr;
	}

	if (auto *const_aggregate = llvm::dyn_cast<llvm::ConstantAggregate>(ptr))
	{
		return const_aggregate->getAggregateElement(index);
	}

	return ctx.builder.CreateGEP(get_llvm_type(ctx.llvm_ctx), ptr, {ctx.builder.getInt32(0), ctx.builder.getInt32(index)});
}

std::optional<pars::MemberInfo> pars::Struct::get_member(std::string_view symbol) const
{
	auto iter = std::find_if(fields.begin(), fields.end(),
		[symbol](auto &element) { return element.symbol.name == symbol; });

	if (iter == fields.end())
	{
		return std::nullopt;
	}

	return MemberInfo{iter->symbol.name, iter->type};
}

llvm::Type * pars::Str::get_llvm_type(llvm::LLVMContext *ctx) const
{
	return llvm::PointerType::get(llvm::Type::getInt8Ty(*ctx), 0);
}

llvm::Value * pars::Str::get_default_value(llvm::LLVMContext *ctx) const
{
	auto *ptr_type = get_llvm_type(ctx);
	return llvm::ConstantPointerNull::get((llvm::PointerType*)ptr_type);
}

llvm::Value * pars::RangeType::op_in(EmitCtx &ctx, llvm::Value *lhs, llvm::Value *rhs) const
{
	return Type::op_in(ctx, lhs, rhs);
}

std::span<pars::Type *> pars::RangeType::get_iter_bindings() const
{
	static Type* binding[1] = {const_cast<Integer*>(&I32Type)};
	return binding;
}

llvm::Value * pars::RangeType::iter_emit_init(EmitCtx &ctx, Expr *iterable, std::span<llvm::Value *> vars) const
{
	auto *bin = dynamic_cast<BinaryExpr*>(iterable);

	auto *min = bin->left->emit(ctx);

	return ctx.builder.CreateStore(min, vars[0]);
}

llvm::Value * pars::RangeType::iter_emit_condition(EmitCtx &ctx, Expr *iterable, std::span<llvm::Value *> vars) const
{
	auto *bin = dynamic_cast<BinaryExpr*>(iterable);

	auto *max = bin->right->emit(ctx);

	using enum TokenType;

	auto op = bin->op == DotDotEqual ? LessEqual : Less;

	auto *v = ctx.builder.CreateLoad(I32Type.get_llvm_type(ctx.llvm_ctx), vars[0]);

	return bin->left->type->op_binary(ctx, op, v, max);
}

llvm::Value * pars::RangeType::iter_emit_update(EmitCtx &ctx, Expr *iterable, std::span<llvm::Value *> vars) const
{
	auto *v = ctx.builder.CreateLoad(I32Type.get_llvm_type(ctx.llvm_ctx), vars[0]);

	auto *inc = I32Type.op_binary(ctx, TokenType::Plus, v, llvm::ConstantInt::get(*ctx.llvm_ctx, llvm::APInt(32, 1)));

	return ctx.builder.CreateStore(inc, vars[0]);
}

llvm::Function * pars::FnSignature::emit(EmitCtx &ctx, std::string_view name, FnFlags flags) const
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

llvm::Value* pars::FnType::emit(EmitCtx &ctx, EmitParams params)
{
	auto *fn = signature.emit(ctx, get_fn_name(), flags);

	for (auto i = 0; auto &arg : fn->args())
	{
		arg.setName(signature.parameters[i++]->symbol.name);
	}

	auto *original_bb = ctx.builder.GetInsertBlock();

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

			if (signature.return_type->is_equal(&VoidType) && ctx.builder.GetInsertBlock()->getTerminator() == nullptr)
			{
				create_safe_void_ret(ctx);
			}
		}
	}

	ctx.builder.SetInsertPoint(original_bb);

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

std::string_view pars::FnType::get_fn_name() const
{
	if (mangled_name.empty())
	{
		return symbol.name;
	}

	return mangled_name;
}

llvm::Value * pars::FnType::get_default_value(llvm::LLVMContext *ctx) const
{
	return llvm::ConstantPointerNull::get((llvm::PointerType*)get_llvm_type(ctx));
}

llvm::FunctionType * pars::FnType::get_fn_llvm_type(llvm::LLVMContext *ctx) const
{
	thread_local std::vector<llvm::Type*> llvm_type_cache;

	llvm_type_cache.clear();

	for (auto *param : signature.parameters)
	{
		llvm_type_cache.emplace_back(param->type->get_llvm_type(ctx));
	}

	return llvm::FunctionType::get(signature.return_type_meta.type->get_llvm_type(ctx), llvm_type_cache, false);
}

llvm::Type * pars::FnType::get_llvm_type(llvm::LLVMContext *ctx) const
{
	return llvm::PointerType::get(get_fn_llvm_type(ctx), 0);
}

llvm::Value * pars::FnType::op_call(EmitCtx &ctx, llvm::Value *callable, llvm::ArrayRef<llvm::Value *> args) const
{
	if (callable == nullptr || !symbol.name.empty())
	{
		auto name = get_fn_name();

		auto *fn = ctx.module->getFunction(name);

		if (fn == nullptr)
		{
			fn = signature.emit(ctx, name, flags);
		}

		return ctx.builder.CreateCall(fn, args);
	}

	auto *fn_type = get_fn_llvm_type(ctx.llvm_ctx);

	return ctx.builder.CreateCall(fn_type, callable, args);
}

bool pars::FnType::is_equal(Type const *other) const
{
	auto other_fn = dynamic_cast<FnType const*>(other);

	auto ret_match = [&]()
	{
		return signature.return_type_meta.type->is_equal(other_fn->signature.return_type_meta.type);
	};

	if (other_fn == nullptr
		|| other_fn->signature.parameters.size() != signature.parameters.size()
		|| !ret_match())
	{
		return false;
	}

	for (auto i = 0; auto *param : signature.parameters)
	{
		auto *other_param = other_fn->signature.parameters[i];

		if (!param->type->is_equal(other_param->type) || other_param->type_meta.mut_set != param->type_meta.mut_set)
		{
			return false;
		}

		i++;
	}

	return ret_match();
}

#include "type.hpp"

bool pars::PrimitiveType::is_equal(Type *other)
{
	auto *other_primitive = dynamic_cast<PrimitiveType*>(other);

	return other != nullptr && other_primitive->kind == kind;
}

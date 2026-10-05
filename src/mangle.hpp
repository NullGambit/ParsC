#pragma once

#include <string>
#include <string_view>

namespace pars
{
	template<class Params, class GetTypeFn>
	void mangle(std::string_view symbol, const Params &params, Type *parent_type, std::string &buffer, GetTypeFn get_type_fn)
	{
		buffer += "?";

		if (parent_type != nullptr)
		{
			buffer += parent_type->get_type_name();
			buffer += "::";
		}

		buffer += symbol;

		for (auto &param : params)
		{
			buffer += '_';

			// TODO maybe add a method such as Type::get_mangled_name for type mangling especially
			auto *type = get_type_fn(param);

			buffer += type->get_type_name();
		}
	}
}

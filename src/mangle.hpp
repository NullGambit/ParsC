#pragma once

#include <string>
#include <string_view>

namespace pars
{
	template<class Params, class GetTypeFn>
	void mangle(std::string_view symbol, const Params &params, std::string &buffer, GetTypeFn get_type_fn)
	{
		buffer += "?";

		//TODO add module name

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

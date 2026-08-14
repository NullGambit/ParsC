#pragma once

namespace pars
{
	template<class ...A>
	struct overload : A... { using A::operator()...; };

	template<class... A>
	overload(A...) -> overload<A...>;
}
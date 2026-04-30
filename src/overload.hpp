#pragma once

namespace ccc
{
	template<class ...A>
	struct overload : A... { using A::operator()...; };

	template<class... A>
	overload(A...) -> overload<A...>;
}
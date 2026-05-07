#pragma once
#include <optional>
#include <string_view>

namespace pars
{
	struct SourceFile
	{
		u16 id;
		std::string_view path;
		std::string_view contents;
	};

	std::optional<SourceFile> load_file(std::string_view path);

	SourceFile get_source(u16 id);
}

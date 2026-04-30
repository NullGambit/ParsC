#include "file_manager.hpp"

#include <string>
#include <vector>

#include "util/io.hpp"

static std::vector<pars::SourceFile> g_file_index;
static std::string g_file_buffers;

std::optional<pars::SourceFile> pars::load_file(std::string_view path)
{
	auto start = g_file_buffers.size();
	auto id = g_file_index.size();

	auto ok = read_file(path, g_file_buffers);

	if (!ok)
	{
		return std::nullopt;
	}

	auto view = std::string_view{g_file_buffers.data() + start, g_file_buffers.size() - start};

	auto src = SourceFile
	{
		.id = static_cast<u16>(id),
		.path = path,
		.contents = view,
	};

	g_file_index.emplace_back(src);

	return src;
}

pars::SourceFile pars::get_source(u16 id)
{
	return g_file_index[id];
}

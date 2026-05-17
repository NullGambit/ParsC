#include "file_manager.hpp"

#include <string>
#include <vector>

#include "memory/arena.hpp"
#include "memory/defs.hpp"
#include "util/fmt.hpp"
#include "util/io.hpp"

// if any project exceeds this absurd file size then uh send me an email
// also because of how virtual memory works this will not cause high memory usage
constexpr auto FILE_BUFFERS_MAX_SIZE = MB(512);

static std::vector<pars::SourceFile> g_file_index;
static pars::Arena g_file_buffers;

std::optional<pars::SourceFile> pars::load_file(std::string_view path)
{
	if (g_file_buffers.memory == nullptr)
	{
		g_file_buffers.init(FILE_BUFFERS_MAX_SIZE);
	}

	auto start = g_file_buffers.occupied;
	auto id = g_file_index.size();

	auto ok = read_file(path, g_file_buffers);

	if (!ok)
	{
		return std::nullopt;
	}

	auto view = std::string_view{(char*)(g_file_buffers.memory + start), g_file_buffers.occupied - start};

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

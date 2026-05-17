#pragma once

#include <optional>
#include <string>
#include <filesystem>

namespace pars
{
    struct Arena;

    bool read_file(const std::filesystem::path &path, Arena &buffer);
}

#pragma once

#include <optional>
#include <string>
#include <filesystem>

namespace std::filesystem::__cxx11
{
    class path;
}

namespace pars
{
    std::optional<std::string> read_file(const std::filesystem::path &path);
}

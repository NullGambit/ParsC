#pragma once

#include <optional>
#include <string>
#include <filesystem>

namespace pars
{
    bool read_file(const std::filesystem::path &path, std::string &buffer);
}

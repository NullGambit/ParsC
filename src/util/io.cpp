#include "io.hpp"

std::optional<std::string> pars::read_file(const std::filesystem::path &path)
{
    auto *file = fopen(path.c_str(), "r");

    if (file == nullptr)
    {
        return std::nullopt;
    }

    auto size = std::filesystem::file_size(path);

    std::string buffer;

    buffer.resize(size);

    fread(buffer.data(), 1, size, file);

    return buffer;
}

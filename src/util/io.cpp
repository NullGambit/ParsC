#include "io.hpp"

bool pars::read_file(const std::filesystem::path &path, std::string &buffer)
{
    auto *file = fopen(path.c_str(), "r");

    if (file == nullptr)
    {
        return false;
    }

    auto size = std::filesystem::file_size(path);

    buffer.resize(buffer.size() + size);

    fread(buffer.data(), 1, size, file);

    return true;
}

#include "io.hpp"

#include "memory/arena.hpp"

bool pars::read_file(const std::filesystem::path &path, Arena &buffer)
{
    auto *file = fopen(path.c_str(), "r");

    if (file == nullptr)
    {
        return false;
    }

    auto size = std::filesystem::file_size(path);

    fread(buffer.memory + buffer.occupied, 1, size, file);

    buffer.occupied += size;

    return true;
}

#include "arena.hpp"

#include <cstring>

#include "virtual_alloc.hpp"

void pars::Arena::init(u64 capacity)
{
	this->capacity = capacity;
	occupied = 0;
	memory = pars::virtual_alloc(capacity);
}

void pars::Arena::free()
{
	pars::virtual_free(memory);
}

u8* pars::Arena::write(const u8 *bytes, u64 size)
{
	auto *current_mem = memory + occupied;

	memcpy(current_mem, bytes, size);

	occupied += size;

	return current_mem;
}

void pars::Arena::write(std::string_view sv)
{
	write(reinterpret_cast<const u8*>(sv.data()), sv.size());
}

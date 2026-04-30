#include "linux_virtual_alloc.hpp"

#include <sys/mman.h>
#include <cstring>

#include <unistd.h>

inline size_t align_to(size_t value, size_t alignment)
{
	return value + (alignment - 1) & ~(alignment - 1);
}

u8* pars::linux_virtual_alloc(size_t size)
{
	constexpr auto n = sizeof(size_t);

	auto page_size = sysconf(_SC_PAGESIZE);

	size = align_to(size + n, page_size);

	auto *ptr = (u8*)mmap(nullptr, size + n, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if (ptr == (void *) -1)
	{
		return nullptr;
	}

	memcpy(ptr, &size, n);

	return ptr + n;
}

void pars::linux_virtual_free(u8* ptr)
{
	if (!ptr)
	{
		return;
	}

	auto size = (size_t)(ptr - sizeof(size_t));

	munmap(ptr, size);
}
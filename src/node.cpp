#include "node.hpp"

#include <cstring>

#include "memory/virtual_alloc.hpp"

#define KB(x) ((x) * 1024UL)
#define MB(x) ((x) * 1024UL * 1024UL)
#define GB(x) ((x) * 1024UL * 1024UL * 1024UL)

constexpr auto BLOCK_SIZE = MB(32);

// all node memory is allocated in large memory blocks for pointer stability
// nothing is ever reallocated
struct MemoryBlock
{
	u8 *ptr;
	size_t offset {};
	MemoryBlock *next;

	void init()
	{
		ptr = pars::linux_virtual_alloc(BLOCK_SIZE);
	}
};

static MemoryBlock g_root_block;
static MemoryBlock *g_current_block = nullptr;

u8* get_memory(size_t size)
{
	if (g_current_block == nullptr)
	{
		g_root_block.init();
		g_current_block = &g_root_block;
	}

	if (g_current_block->offset + size >= BLOCK_SIZE)
	{
		auto block = new MemoryBlock;

		block->init();

		g_current_block->next = block;

		g_current_block = block;
	}

	auto offset = g_current_block->offset;

	g_current_block->offset += size;

	return g_current_block->ptr + offset;
}

u8 * pars::alloc_node(u32 size)
{
	return get_memory(size);
}

void pars::free_memory_blocks()
{
	auto *block = &g_root_block;

	while (block->next)
	{
		virtual_free(block->ptr);

		auto *temp_block = block;

		block = block->next;

		if (block != &g_root_block)
		{
			delete temp_block;
		}
	}
}

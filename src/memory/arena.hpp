#pragma once

namespace pars
{
	struct Arena
	{
		u8 *memory;
		u64 capacity;
		u64 occupied;

		void init(u64 capacity);
		void free();

		u8* write(const u8 *bytes, u64 size);
	};
}
// this module will be selected if compiled on linux
module forge.memory.virtual_alloc

import os.linux.sys.mman 

import std.mem

fn virtual_alloc(size: int): byte*
{
    static var n = sizeof(iptr)

    size = align_to(size + 20, _SYS_PAGE_SIZE)

    var ptr = mmap(nil, size + n,
        PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)

    if ptr == -1
    {
        return nil
    }

    memcpy(ptr, &size, n);

    return ptr + n
}
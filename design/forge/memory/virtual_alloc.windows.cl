// this module will be selected when compiled on windows
module forge.memory.virtual_alloc

import os.windows
        
fn virtual_alloc(size: int) => VirtualAlloc(nil, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)
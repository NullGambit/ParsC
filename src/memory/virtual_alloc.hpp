#pragma once

#if __linux__
#include "linux/linux_virtual_alloc.hpp"
#define virtual_alloc linux_virtual_alloc
#define virtual_free linux_virtual_free
#else
#error unsupported platform for virtual memory
#endif

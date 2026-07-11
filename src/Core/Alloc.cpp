#include "Core/Alloc.hpp"
#include "Core/Assert.hpp"

#include <limits>

namespace core
{
MallocHook malloc_func = ::malloc;
FreeHook free_func = ::free;

std::atomic_size_t memory_allocated_bytes = 0;
std::atomic_size_t memory_freed_bytes = 0;

void *alloc(size_t n)
{
    ASSERT_V(n < std::numeric_limits<uint32_t>::max(), "Trying to allocate too much memory: {} bytes", n);
    void *p = malloc_func(n);
    ASSERT_V(p != nullptr, "Out of memory for {}-sized allocation", n);
    return p;
}

void free(void *p)
{
    free_func(p);
}
}; // namespace core

size_t core::get_memory_usage()
{
    return core::memory_allocated_bytes - core::memory_freed_bytes;
}

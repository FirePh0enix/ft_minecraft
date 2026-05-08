#include "Core/Alloc.hpp"

namespace core
{
MallocHook malloc_func = malloc;
FreeHook free_func = free;

std::atomic_size_t memory_allocated_bytes = 0;
std::atomic_size_t memory_freed_bytes = 0;
}; // namespace core

size_t core::get_memory_usage()
{
    return core::memory_allocated_bytes - core::memory_freed_bytes;
}

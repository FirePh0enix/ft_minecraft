#include "Core/Alloc.hpp"

namespace core
{
MallocHook malloc_func = malloc;
FreeHook free_func = free;
}; // namespace core

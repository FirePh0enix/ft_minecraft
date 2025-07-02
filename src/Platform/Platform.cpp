#include "Platform/Platform.hpp"

#if defined(__platform_linux)
#include <sys/resource.h>
#include <unistd.h>
#endif

size_t get_current_rss()
{
#if defined(__platform_linux)
    long rss = 0L;
    FILE *fp = fopen("/proc/self/statm", "r");
    if (fp == nullptr)
        return (size_t)0;
    if (fscanf(fp, "%*s%ld", &rss) != 1)
    {
        fclose(fp);
        return (size_t)0;
    }
    fclose(fp);
    return (size_t)rss * (size_t)sysconf(_SC_PAGESIZE);
#elif defined(__platform_macos)
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  (task_info_t)&info, &infoCount) != KERN_SUCCESS)
        return (size_t)0;
    return (size_t)info.resident_size;
#elif defined(__platform_windows)
    PROCESS_MEMORY_COUNTERS info;
    GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
    return (size_t)info.WorkingSetSize;
#endif

    return 0;
}

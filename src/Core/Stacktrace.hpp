#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#ifndef STACKTRACE_SIZE
#define STACKTRACE_SIZE 10
#endif

struct Stacktrace
{
    struct Frame
    {
        const char *function;
        const char *filename;
        uint32_t line;
    };

    bool stop_at_main = true;

    Frame frames[STACKTRACE_SIZE];
    size_t length = 0;
    size_t total_length = 0;
    bool non_exhaustive = false;
    bool early_end = false;

    static const Stacktrace& current();
    static void record();

    void print(FILE *fp = stderr, size_t skip_frame = 0) const;
};

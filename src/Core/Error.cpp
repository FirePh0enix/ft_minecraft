#include "Error.hpp"

#if defined(__TARGET_LINUX__) || defined(__TARGET_APPLE__)

#include <csignal>
#include <execinfo.h>

#endif

#include <cxxabi.h>

static thread_local StackTrace stacktrace;
static backtrace_state *bt_state;

int bt_callback(StackTrace *st, uintptr_t, const char *filename, int lineno, const char *function)
{
    if (st->length >= st->frames.size())
    {
        st->non_exhaustive = true;
        return 0;
    }

    st->non_exhaustive = false;

    if (st->eary_end)
    {
        return 0;
    }

    if (function != nullptr && st->stop_at_main && !std::strcmp(function, "main"))
    {
        st->eary_end = true;
    }

    const char *func_name = function;
    int status;
    char *demangled = abi::__cxa_demangle(function, nullptr, nullptr, &status);
    if (status == 0)
    {
        func_name = demangled;
    }

    st->frames[st->length] = {.function = func_name, .filename = filename, .line = (uint32_t)lineno};
    st->length += 1;

    return 0;
}

void bt_error_callback(void *, const char *msg, int errnum)
{
    (void)msg;
    (void)errnum;
}

const StackTrace& StackTrace::current()
{
    backtrace_full(bt_state, 1, (backtrace_full_callback)bt_callback, (backtrace_error_callback)bt_error_callback, &stacktrace);
    return stacktrace;
}

void StackTrace::print(FILE *fp, size_t skip_frame) const
{
    for (size_t i = skip_frame; i < length; i++)
    {
        const Frame& frame = frames[i];
        std::println(fp, "#{}: {} in {}:{}", i - skip_frame, frame.function ? frame.function : "???", frame.filename ? frame.filename : "???", frame.line);
    }

    if (non_exhaustive)
        std::println(fp, "(end)");
}

#if defined(__TARGET_LINUX__) || defined(__TARGET_APPLE__)

void signal_handler(int sig)
{
    const char *signal_name = strsignal(sig);
    const StackTrace& st = StackTrace::current();

    std::println(stderr, "Received signal: {}\n", signal_name);
    st.print(stderr, 0);

    exit(1);
}

#endif

void initialize_error_handling(const char *filename)
{
    bt_state = backtrace_create_state(filename, 1, nullptr, nullptr);

#if defined(__TARGET_LINUX__) || defined(__TARGET_APPLE__)
    signal(SIGSEGV, signal_handler);
    signal(SIGABRT, signal_handler);
    signal(SIGILL, signal_handler);
#endif
}

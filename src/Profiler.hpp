#pragma once

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>

#define TracySetThreadName(NAME) tracy::SetThreadName(NAME)

#else

#define TracySetThreadName(NAME)

#define ZoneScopedN(NAME)
#define FrameMark
#define ZoneScoped

#endif

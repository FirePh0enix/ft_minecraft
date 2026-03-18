#pragma once

// Only way to force function inlining.
#define ALWAYS_INLINE inline __attribute__((always_inline))

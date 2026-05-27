#include "Core/Result.hpp"
#include "Core/Error.hpp"

#include <doctest/doctest.h>

TEST_CASE("Result")
{
    Result<void> void_value;
    CHECK(void_value.has_value());

    Result<void> error = Error(ErrorKind::OutOfMemory);
    CHECK(error.has_error());
}

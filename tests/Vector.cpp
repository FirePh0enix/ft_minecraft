#include "Core/Containers/Vector.hpp"

#include <doctest/doctest.h>

TEST_CASE("Vector ints")
{
    Vector<int> v;
    CHECK(v.append(42));
    CHECK(v.append(12));
    CHECK(v.append(5));
    CHECK(v.append(9));
}

TEST_CASE("Vector strings")
{
    Vector<String> strings;
    CHECK(strings.append("hello world everyone, this is long string"));
    CHECK(strings.append("hello world everyone, this is long string"));
    CHECK(strings.append("hello world everyone, this is long string"));
    CHECK(strings.append("hello world everyone, this is long string"));
}

TEST_CASE("Vector reserve()")
{
    Vector<String> strings;
    EXPECT(strings.reserve(1));
    CHECK_EQ(strings.references(), 1);
}

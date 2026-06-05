#include "Core/Containers/Vector.hpp"

#include <doctest/doctest.h>

TEST_CASE("Vector ints")
{
    Vector<int> v;
    v.append(42);
    v.append(12);
    v.append(5);
    v.append(9);
}

TEST_CASE("Vector strings")
{
    Vector<String> strings;
    strings.append("hello world everyone, this is long string");
    strings.append("hello world everyone, this is long string");
    strings.append("hello world everyone, this is long string");
    strings.append("hello world everyone, this is long string");
}

TEST_CASE("Vector reserve()")
{
    Vector<String> strings;
    strings.reserve(1);
    CHECK_EQ(strings.references(), 1);
}

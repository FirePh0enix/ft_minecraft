#include "Variant.hpp"

#include <doctest/doctest.h>

TEST_CASE("Copy constructor when tag is String")
{
    String s("foo bar");
    Variant v = s;
    Variant v2 = v;

    String s2 = v.get_unchecked<String>();

    CHECK_EQ(s, s2);
}

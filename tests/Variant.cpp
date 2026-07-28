#include "Variant.hpp"

#include <doctest/doctest.h>

TEST_CASE("Copy constructor when tag is String")
{
    std::string s = "foo bar";
    Variant v = s;
    Variant v2 = v;

    std::string s2 = v.get_unchecked<std::string>();

    CHECK_EQ(s, s2);
}

TEST_CASE("Variant with Array")
{
    std::vector<std::string> strings;
    strings.push_back("hello world with allocation");

    Variant v = std::span(strings);
}

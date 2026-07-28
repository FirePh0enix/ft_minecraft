#include "Core/Option.hpp"

#include <doctest/doctest.h>

TEST_CASE("Simple Option")
{
    Option<int> opt = 43;
    CHECK(opt.has_value());

    opt = None;
    CHECK(!opt.has_value());

    Option<int> opt2;
    CHECK(!opt.has_value());
}

TEST_CASE("Option with complex types")
{
    Option<std::string> s = std::string("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    CHECK(s.has_value());

    s = None;
    CHECK(!s.has_value());
}

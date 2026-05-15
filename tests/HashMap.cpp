#include "Core/Containers/HashMap.hpp"
#include "Core/Result.hpp"
#include "Core/String.hpp"

#include <doctest/doctest.h>

TEST_CASE("HashMap ordering")
{
    HashMap<int, int> maps;
    EXPECT(maps.put(1, 1));
    CHECK(maps.get(1) == 1);

    EXPECT(maps.put(1, 2));
    CHECK(maps.get(1) == 2);

    EXPECT(maps.put(2, 3));
    CHECK(maps.get(2) == 3);
    CHECK(maps.get(1) == 2);

    CHECK(maps.size() == 2);
}

TEST_CASE("HashMap with String")
{
    HashMap<String, String> maps;
    EXPECT(maps.put("hello", "hello world"));
    EXPECT(maps.put("bar", "foo"));

    CHECK(maps.get("hello") == "hello world");
    CHECK(maps.get("bar") == "foo");
}

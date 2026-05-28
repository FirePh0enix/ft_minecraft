#include "Core/Containers/HashMap.hpp"
#include "Core/Containers/Map.hpp"
#include "Core/Result.hpp"
#include "Core/String.hpp"

#include <doctest/doctest.h>

TEST_CASE("HashMap ordering")
{
    HashMap<int, int> maps;
    CHECK(maps.put(1, 1).has_value());
    CHECK(maps.get(1) == 1);

    CHECK(maps.put(1, 2).has_value());
    CHECK(maps.get(1) == 2);

    CHECK(maps.put(2, 3).has_value());
    CHECK(maps.get(2) == 3);
    CHECK(maps.get(1) == 2);

    CHECK(maps.size() == 2);
}

TEST_CASE("HashMap with String")
{
    HashMap<String, String> maps;
    CHECK(maps.put("hello", "hello world").has_value());
    CHECK(maps.put("bar", "foo").has_value());

    CHECK(maps.get("hello") == "hello world");
    CHECK(maps.get("bar") == "foo");
}

TEST_CASE("HashMap copy with complex types")
{
    HashMap<String, String> maps;
    CHECK(maps.put("hello", "hello world").has_value());
    CHECK(maps.put("bar", "foo").has_value());
    CHECK_EQ(maps.get("hello"), "hello world");
    CHECK_EQ(maps.get("bar"), "foo");

    HashMap<String, String> maps2;
    maps2 = maps;
}

TEST_CASE("HashMap insertion out of order")
{
    HashMap<uint32_t, int> map;
    CHECK(map.put(0x8d17a2cc, 0).has_value());
    CHECK_EQ(map.pairs()[0].key, 0x8d17a2cc);

    CHECK(map.put(0x1e826268, 0).has_value());
    CHECK_EQ(map.pairs()[0].key, 0x1e826268);
    CHECK_EQ(map.pairs()[1].key, 0x8d17a2cc);
}

struct Foo
{
    int v;
};

TEST_CASE("HashMap big insertion")
{
    HashMap<String, Vector<Foo>> map;
    CHECK(map.put("forward", Vector<Foo>()));
    CHECK(map.put("backward", Vector<Foo>()));
    CHECK(map.put("left", Vector<Foo>()));
    CHECK(map.put("right", Vector<Foo>()));
    CHECK(map.put("jump", Vector<Foo>()));
    CHECK(map.put("down", Vector<Foo>()));
    CHECK(map.put("attack", Vector<Foo>()));
    CHECK(map.put("interact", Vector<Foo>()));
    CHECK(map.put("escape", Vector<Foo>()));
    CHECK(map.put("open_inventory", Vector<Foo>()));
    CHECK(map.put("1", Vector<Foo>()));
    CHECK(map.put("2", Vector<Foo>()));
    CHECK(map.put("3", Vector<Foo>()));
    CHECK(map.put("4", Vector<Foo>()));
    CHECK(map.put("5", Vector<Foo>()));
    CHECK(map.put("6", Vector<Foo>()));
    CHECK(map.put("7", Vector<Foo>()));
    CHECK(map.put("8", Vector<Foo>()));
    CHECK(map.put("9", Vector<Foo>()));
}

TEST_CASE("Map erase")
{
    Map<int, int> map;
    CHECK(map.put(3, 3));
    CHECK(map.put(8, 8));
    CHECK(map.put(5, 5));
    CHECK(map.put(1, 1));

    map.erase(3);

    CHECK(map.pairs()[0].key == 1);
    CHECK(map.pairs()[1].key == 5);
    CHECK(map.pairs()[2].key == 8);
}

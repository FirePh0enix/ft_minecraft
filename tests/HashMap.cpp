#include "Core/Containers/HashMap.hpp"
#include "Core/Containers/Vector.hpp"
#include "Core/Alloc.hpp"
#include "Core/Containers/Map.hpp"
#include "Core/String.hpp"

#include <doctest/doctest.h>

TEST_CASE("HashMap ordering")
{
    HashMap<int, int> maps;
    maps.put(1, 1);
    CHECK(maps.get(1) == 1);

    maps.put(1, 2);
    CHECK(maps.get(1) == 2);

    maps.put(2, 3);
    CHECK(maps.get(2) == 3);
    CHECK(maps.get(1) == 2);

    CHECK(maps.size() == 2);
}

TEST_CASE("HashMap with String")
{
    HashMap<String, String> maps;
    maps.put("hello", "hello world");
    maps.put("bar", "foo");

    CHECK(maps.get("hello") == "hello world");
    CHECK(maps.get("bar") == "foo");
}

TEST_CASE("HashMap copy with complex types")
{
    HashMap<String, String> maps;
    maps.put("hello", "hello world");
    maps.put("bar", "foo");
    CHECK_EQ(maps.get("hello"), "hello world");
    CHECK_EQ(maps.get("bar"), "foo");

    HashMap<String, String> maps2;
    maps2 = maps;
}

TEST_CASE("HashMap insertion out of order")
{
    HashMap<uint32_t, int> map;
    map.put(0x8d17a2cc, 0);
    CHECK_EQ(map.pairs()[0].key, 0x8d17a2cc);

    map.put(0x1e826268, 0);
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
    map.put("forward", Vector<Foo>());
    map.put("backward", Vector<Foo>());
    map.put("left", Vector<Foo>());
    map.put("right", Vector<Foo>());
    map.put("jump", Vector<Foo>());
    map.put("down", Vector<Foo>());
    map.put("attack", Vector<Foo>());
    map.put("interact", Vector<Foo>());
    map.put("escape", Vector<Foo>());
    map.put("open_inventory", Vector<Foo>());
    map.put("1", Vector<Foo>());
    map.put("2", Vector<Foo>());
    map.put("3", Vector<Foo>());
    map.put("4", Vector<Foo>());
    map.put("5", Vector<Foo>());
    map.put("6", Vector<Foo>());
    map.put("7", Vector<Foo>());
    map.put("8", Vector<Foo>());
    map.put("9", Vector<Foo>());
}

struct Bar
{
    int *ptr;

    Bar()
        : ptr(alloc<int>(0))
    {
    }

    ~Bar()
    {
        destroy(ptr);
    }
};

TEST_CASE("HashMap with allocating values")
{
    HashMap<int, Bar> bars;
    bars.put(33, Bar());
}

TEST_CASE("HashMap with reallocation")
{
    HashMap<int, HashMap<String, String>> bars;
    for (int i = 0; i < 5; i++)
        for (size_t j = 0; j < 1; j++)
            bars.get_or_put(i, {})->put(format("{}", j), "aaa");
}

TEST_CASE("Map erase")
{
    Map<int, int> map;
    map.put(3, 3);
    map.put(8, 8);
    map.put(5, 5);
    map.put(1, 1);

    map.erase(3);

    CHECK_EQ(map.pairs()[0].key, 1);
    CHECK_EQ(map.pairs()[1].key, 5);
    CHECK_EQ(map.pairs()[2].key, 8);
}

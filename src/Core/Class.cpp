#include "Core/Class.hpp"

#include <doctest/doctest.h>

#include <cstring>
#include <print>

#ifndef DOCTEST_CONFIG_DISABLE

class Foo : public Object
{
    CLASS(Foo, Object);
};

class Bar : public Foo
{
    CLASS(Bar, Foo);
};

#endif

TEST_CASE("Class")
{
    Foo::register_class();
    Bar::register_class();

    CHECK(!strcmp(Foo::get_static_class_name(), "Foo"));

    const std::vector<uint32_t>& classes = Bar::get_static_classes();
    CHECK(classes.size() == 3);
    CHECK(classes[0] == Object::get_static_hash_code());
    CHECK(classes[1] == Foo::get_static_hash_code());
    CHECK(classes[2] == Bar::get_static_hash_code());

    Foo *obj = new Bar();
    CHECK(obj->is<Bar>());
    CHECK(obj->is<Foo>());
}

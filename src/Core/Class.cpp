#include "Core/Class.hpp"

#include <cstring>

#ifdef DOCTEST_CONFIG_ENABLE

#include <doctest/doctest.h>

class Foo : public Object
{
    CLASS(Foo, Object);
};

class Bar : public Foo
{
    CLASS(Bar, Foo);
};

TEST_CASE("Class")
{
    Foo::register_class();
    Bar::register_class();

    CHECK(!strcmp(Foo::get_static_class_name(), "Foo"));

    const View<ClassHashCode>& classes = Bar::get_static_classes();
    CHECK(classes.size() == 3);
    CHECK(classes[0] == Object::get_static_hash_code());
    CHECK(classes[1] == Foo::get_static_hash_code());
    CHECK(classes[2] == Bar::get_static_hash_code());

    Foo *obj = new Bar();
    CHECK(obj->is<Bar>());
    CHECK(obj->is<Foo>());

    delete obj;
}

#endif

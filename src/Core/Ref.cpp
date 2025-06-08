#include "Core/Ref.hpp"
#include "Core/Class.hpp"

#include <doctest/doctest.h>

#ifndef DOCTEST_CONFIG_DISABLE

class Foo : public Object
{
    CLASS(Foo, Object);

public:
    virtual ~Foo() {}
};

class Bar : public Foo
{
    CLASS(Bar, Foo);

public:
    virtual ~Bar() {}
};

class Bleep : public Object
{
    CLASS(Bleep, Foo);

public:
    virtual ~Bleep() {}
};

TEST_CASE("Ref sanity checks")
{
    Foo::register_class();

    Ref<Foo> foo_null;
    CHECK(foo_null.is_null());
    CHECK(foo_null.ptr() == nullptr);
    CHECK(foo_null.references() == nullptr);

    Ref<Foo> foo = make_ref<Foo>();
    CHECK(*foo.references() == 1);

    {
        Ref<Foo> foo2 = foo;
        CHECK(*foo2.references() == 2);
    }

    CHECK(*foo.references() == 1);

    foo = nullptr;
    CHECK(foo.is_null());

    Ref<Foo> foo3 = make_ref<Foo>();
    Ref<Foo> foo4 = make_ref<Foo>();
    foo3 = foo4;
    foo4 = nullptr;

    CHECK(*foo3.references() == 1);
    CHECK(foo4.is_null());

    Ref<Foo> foo5 = make_ref<Foo>();
    {
        Ref<Foo> foo5_1 = foo5;
        Ref<Foo> foo5_2 = foo5;
        Ref<Foo> foo5_3 = foo5;
        Ref<Foo> foo5_4 = foo5;
        Ref<Foo> foo5_5 = foo5;

        {
            Ref<Foo> foo5_5 = foo5;
            Ref<Foo> foo5_6 = foo5;
            Ref<Foo> foo5_7 = foo5;
            Ref<Foo> foo5_8 = foo5;
            Ref<Foo> foo5_9 = foo5;
        }

        Ref<Foo> foo5_6(foo5_5);
        Ref<Foo> foo5_7(foo5_6);
    }

    CHECK(*foo5.references() == 1);
    foo5 = nullptr;

    // Self assignment
    Ref<Foo> foo6 = make_ref<Foo>();
    Ref<Foo> foo7 = foo6;

    foo6 = foo7;

    CHECK(*foo6.references() == 2);
}

TEST_CASE("Ref casting")
{
    Foo::register_class();
    Bar::register_class();
    Bleep::register_class();

    // Downcasting
    Ref<Bleep> bleep = make_ref<Bleep>();
    Ref<Object> object = bleep.cast_to<Object>();
    CHECK(!object.is_null());

    // Valid upcasting
    Ref<Foo> foo = object.cast_to<Foo>();
    CHECK(!object.is_null());

    // Invalid upcasting
    Ref<Bar> bar = foo.cast_to<Bar>();
    CHECK(bar.is_null());
}

#endif

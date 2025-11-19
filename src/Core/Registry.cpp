#include "Core/Registry.hpp"
#include "Core/Assert.hpp"

CoreRegistry CoreRegistry::s_singleton;

void CoreRegistry::register_struct(const char *name, Struct s)
{
    m_structs[name] = s;
}

Struct CoreRegistry::get_struct(const StringView& name)
{
    ASSERT_V(m_structs.contains(name.data()), "Invalid type {}", name.data());
    return m_structs[name.data()];
}

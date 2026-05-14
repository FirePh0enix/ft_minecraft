#include "Core/Ref.hpp"
#include "Core/Assert.hpp"

void ref_check_null_access(bool cond, const char *class_name)
{
    ASSERT_V(!cond, "Trying to access a null ref of type {}", class_name);
}

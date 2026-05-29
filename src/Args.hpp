#pragma once

#include "Core/Containers/HashMap.hpp"
#include "Core/Containers/Vector.hpp"
#include "Core/Containers/View.hpp"
#include "Core/String.hpp"

enum class ArgType : uint8_t
{
    Bool,
    String,
};

struct ArgInfo
{
    ArgType type = ArgType::Bool;
};

union ArgValue
{
    bool boolean;
    char *string;
};

class Args
{
public:
    void add_arg(const String& name, const ArgInfo& info);

    /**
     * @brief Parse input arguments.
     */
    void parse(char **argv, int argc);

    ArgValue get_arg(const String& name)
    {
        return m_values.get(name).get();
    }

    bool has(const String& name) const
    {
        return m_values.contains(name);
    }

    View<String> get_other_args() const
    {
        return m_args;
    }

private:
    String m_executable_name;
    HashMap<String, ArgInfo> m_infos;
    HashMap<String, ArgValue> m_values;
    Vector<String> m_args;
};

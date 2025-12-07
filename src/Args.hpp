#pragma once

#include "Core/Containers/View.hpp"
#include "Core/String.hpp"

#include <map>

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
        return m_values[name];
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
    std::map<String, ArgInfo> m_infos;
    std::map<String, ArgValue> m_values;
    std::vector<String> m_args;
};

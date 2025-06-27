#pragma once

#include "Core/Containers/View.hpp"

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
    void add_arg(const std::string& name, const ArgInfo& info);

    /**
     * @brief Parse input arguments.
     */
    void parse(char **argv, int argc);

    ArgValue get_arg(const std::string& name)
    {
        return m_values[name];
    }

    bool has(const std::string& name) const
    {
        return m_values.contains(name);
    }

    View<std::string> get_other_args() const
    {
        return m_args;
    }

private:
    std::map<std::string, ArgInfo> m_infos;
    std::map<std::string, ArgValue> m_values;
    std::vector<std::string> m_args;
};

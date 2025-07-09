#pragma once

#include <iostream>
#include <map>
#include <variant>

using ConfigValue = std::variant<std::string, int64_t, float, bool>;

inline void operator<<(std::ostream& os, const ConfigValue& value)
{
    if (std::holds_alternative<std::string>(value))
    {
        os << std::get<std::string>(value);
    }
    else if (std::holds_alternative<int64_t>(value))
    {
        os << std::get<int64_t>(value);
    }
    else if (std::holds_alternative<float>(value))
    {
        os << std::get<float>(value);
    }
    else if (std::holds_alternative<bool>(value))
    {
        bool b = std::get<bool>(value);
        if (b)
            os << "true";
        else
            os << "false";
    }
}

class ConfigCategory
{
public:
    ConfigCategory();

    void set(const std::string& name, ConfigValue value);

    ConfigValue get(const std::string& name)
    {
        return m_values[name];
    }

    template <typename T>
    T get(const std::string& name)
    {
        return std::get<T>(m_values[name]);
    }

    bool has(const std::string& name) const
    {
        return m_values.contains(name);
    }

    const std::map<std::string, ConfigValue>& values() const
    {
        return m_values;
    }

private:
    std::map<std::string, ConfigValue> m_values;
};

class Config
{
public:
    Config();

    void load_from_str(const std::string& str);

    /**
     * @brief Load a config file.
     */
    void load_from_file(const std::string& filename);

    /**
     * @brief Save the config to `filename`.
     */
    void save_to(const std::string& filename);

    ConfigCategory& get_category(const std::string& name);

private:
    std::map<std::string, ConfigCategory> m_categories;

    ConfigValue get_value(const std::string& str);
};

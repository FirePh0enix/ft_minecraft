#include "Config.hpp"
#include "Core/Logger.hpp"

#include <fstream>

ConfigCategory::ConfigCategory()
{
}

void ConfigCategory::set(const std::string& name, ConfigValue value)
{
    m_values[name] = value;
}

Config::Config()
{
}

void Config::load_from_str(const std::string& str)
{
    std::stringstream ifs(str);

    ConfigCategory category;
    std::string category_name = "general";

    std::string line;
    while (std::getline(ifs, line))
    {
        if (line.size() > 2 && line[0] == '[' && line[line.size() - 1] == ']')
        {
            m_categories[category_name] = category;

            category_name = line.substr(1, line.size() - 2);
            category = m_categories.contains(category_name) ? m_categories[category_name] : ConfigCategory();
        }
        else
        {
            std::string::size_type pos = line.find('=');
            std::string name = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            category.set(name, get_value(value));
        }
    }

    m_categories[category_name] = category;
}

void Config::load_from_file(const std::string& filename)
{
    std::ifstream ifs(filename);

    std::string s((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    load_from_str(s);
}

void Config::save_to(const std::string& filename)
{
    std::ofstream os(filename);

    for (auto& pair : m_categories)
    {
        if (pair.second.values().size() == 0)
            continue;

        os << "[" << pair.first << "]\n";

        for (auto& pair2 : pair.second.values())
        {
            os << pair2.first << "=" << pair2.second;
            os << "\n";
        }
    }
}

ConfigCategory& Config::get_category(const std::string& name)
{
    if (!m_categories.contains(name))
    {
        m_categories[name] = ConfigCategory();
    }
    return m_categories[name];
}

static bool contains_str(const std::string& str, const std::string& chars)
{
    for (size_t i = 0; i < str.size(); i++)
    {
        if (chars.find(str[i]) == std::string::npos)
            return false;
    }
    return true;
}

ConfigValue Config::get_value(const std::string& str)
{
    if (str == "true" || str == "false")
    {
        return ConfigValue((bool)(str == "true"));
    }
    else if (contains_str(str, "0123456789."))
    {
        return ConfigValue(std::stof(str));
    }
    else if (contains_str(str, "0123456789."))
    {
        return ConfigValue((int64_t)std::stoll(str));
    }
    else
    {
        error("config: failed to parse `{}`", str);
        return ConfigValue("");
    }
}

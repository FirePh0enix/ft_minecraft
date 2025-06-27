#include "Args.hpp"
#include "Core/Logger.hpp"

void Args::add_arg(const std::string& name, const ArgInfo& info)
{
    m_infos[name] = info;
}

void Args::parse(char **argv, int argc)
{
    for (int i = 0; i < argc; i++)
    {
        std::string arg = argv[i];

        if (arg.starts_with("--"))
        {
            std::string name = arg.substr(2);

            if (!m_infos.contains(name))
            {
                warn("Unknown argument `{}`, it will be ignored", arg);
                continue;
            }

            const ArgInfo& info = m_infos[name];

            if (info.type != ArgType::Bool && i + 1 >= argc)
            {
                warn("Missing value for argument `{}`, it will be ignored", arg);
                continue;
            }
            else if (info.type == ArgType::Bool)
            {
                m_values[name] = {.boolean = true};
            }
            else
            {
                char *next_argv = argv[i + 1];
                m_values[name] = {.string = next_argv};
            }
        }
        else
        {
            m_args.push_back(arg);
        }
    }
}

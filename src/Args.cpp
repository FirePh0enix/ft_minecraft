#include "Args.hpp"
#include "Core/Logger.hpp"

void Args::add_arg(const String& name, const ArgInfo& info)
{
    m_infos.put(name, info);
}

void Args::parse(char **argv, int argc)
{
    for (int i = 0; i < argc; i++)
    {
        StringView arg = argv[i];

        if (arg.starts_with("--"))
        {
            StringView name = arg.slice(2);

            if (!m_infos.contains(name))
            {
                warn("Unknown argument `{}`, it will be ignored", arg);
                continue;
            }

            ArgInfo info = m_infos.get(name).get();

            if (info.type != ArgType::Bool && i + 1 >= argc)
            {
                warn("Missing value for argument `{}`, it will be ignored", arg);
                continue;
            }
            else if (info.type == ArgType::Bool)
            {
                m_values.put(name, {.boolean = true});
            }
            else
            {
                char *next_argv = argv[i + 1];
                m_values.put(name, {.string = next_argv});
            }
        }
        else
        {
            m_args.append(arg);
        }
    }
}

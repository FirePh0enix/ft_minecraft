#include "Console.hpp"
#include "Core/Print.hpp"
#include "Core/StringView.hpp"

Command::Command(const CommandInfo& info, const std::vector<String>& tokens)
{
    for (size_t i = 1; i < tokens.size(); i++)
    {
        const CmdArgInfo& arg_info = info.args[i - 1];
        switch (arg_info.kind)
        {
        case CmdArgKind::Int:
            m_args[arg_info.name] = CmdArg{.i = std::stol(tokens[i].data())}; // TODO: Replace with a `String` implementation of it.
            break;
        case CmdArgKind::String:
            m_args[arg_info.name] = CmdArg{.s = tokens[i]};
            break;
        }
    }
}

Console::Console()
    : m_buffer()
{
}

void Console::register_command(const String& name, std::vector<CmdArgInfo> args, CommandCallback callback)
{
    m_commands[name] = CommandInfo{.callback = callback, .args = args};
}

void Console::exec()
{
    std::string s = StringView(m_buffer.data(), std::strlen(m_buffer.data()));

    // TODO: Fully replace with `String`.
    const std::string& delimiter = " ";
    std::vector<String> tokens;
    size_t pos = 0;
    std::string token;
    while ((pos = s.find(delimiter)) != std::string::npos)
    {
        token = s.substr(0, pos);
        tokens.push_back(String(token));
        s.erase(0, pos + delimiter.length());
    }
    tokens.push_back(String(s));

    const CommandInfo& info = m_commands[tokens[0]];
    Command command(m_commands[tokens[0]], tokens);
    info.callback(command);
}

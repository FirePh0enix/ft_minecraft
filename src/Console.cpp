#include "Console.hpp"

#include <string>

Command::Command(const CommandInfo& info, const std::vector<std::string_view>& tokens)
{
    for (size_t i = 1; i < tokens.size(); i++)
    {
        const CmdArgInfo& arg_info = info.args[i - 1];
        switch (arg_info.kind)
        {
        case CmdArgKind::Int:
            m_args[arg_info.name] = CmdArg{.i = std::stoll(std::string(tokens[i]))};
            break;
        case CmdArgKind::String:
            m_args[arg_info.name] = CmdArg{.s = std::string(tokens[i])};
            break;
        }
    }
}

Console::Console()
    : m_buffer()
{
}

void Console::register_command(std::string_view name, std::vector<CmdArgInfo> args, CommandCallback callback)
{
    m_commands[std::string(name)] = CommandInfo{.callback = callback, .args = args};
}

void Console::exec()
{
    // StringView(m_buffer, std::strlen(m_buffer)).split(" ");

    std::vector<std::string_view> tokens{std::string_view(m_buffer)}; // = StringView(m_buffer, std::strlen(m_buffer)).split(" ");
    CommandInfo info = m_commands.find(tokens[0])->second;
    Command command(info, tokens);
    info.callback(command);

    m_buffer[0] = '\0';
}

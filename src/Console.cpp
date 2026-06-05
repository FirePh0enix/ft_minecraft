#include "Console.hpp"
#include "Core/StringView.hpp"

Command::Command(const CommandInfo& info, const Vector<StringView>& tokens)
{
    for (size_t i = 1; i < tokens.size(); i++)
    {
        const CmdArgInfo& arg_info = info.args.get_unchecked(i - 1);
        switch (arg_info.kind)
        {
        case CmdArgKind::Int:
            m_args.put(arg_info.name, CmdArg{.i = tokens.get_unchecked(i).parse_int<int64_t>()});
            break;
        case CmdArgKind::String:
            m_args.put(arg_info.name, CmdArg{.s = tokens.get_unchecked(i)});
            break;
        }
    }
}

Console::Console()
    : m_buffer()
{
}

void Console::register_command(const String& name, Vector<CmdArgInfo> args, CommandCallback callback)
{
    m_commands.put(name, CommandInfo{.callback = callback, .args = args});
}

void Console::exec()
{
    Vector<StringView> tokens = StringView(m_buffer, std::strlen(m_buffer)).split(" ");
    CommandInfo info = m_commands.get(tokens.get_unchecked(0)).get();
    Command command(info, tokens);
    info.callback(command);

    m_buffer[0] = '\0';
}

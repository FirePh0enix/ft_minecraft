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
            EXPECT(m_args.put(arg_info.name, CmdArg{.i = std::stol(tokens.get_unchecked(i).data())})); // TODO: Replace with a `String` implementation of it.
            break;
        case CmdArgKind::String:
            EXPECT(m_args.put(arg_info.name, CmdArg{.s = tokens.get_unchecked(i)}));
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
    EXPECT(m_commands.put(name, CommandInfo{.callback = callback, .args = args}));
}

void Console::exec()
{
    Vector<StringView> tokens = StringView(m_buffer, std::strlen(m_buffer)).split(" ");
    CommandInfo info = m_commands.get(tokens.get_unchecked(0)).get();
    Command command(info, tokens);
    info.callback(command);

    m_buffer[0] = '\0';
}

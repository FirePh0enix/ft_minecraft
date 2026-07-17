#pragma once

#include <cstdint>

#include "Core/Containers/HashMap.hpp"
#include "Core/Containers/Vector.hpp"
#include "Core/String.hpp"

enum class CmdArgKind
{
    Int,
    String,
};

struct CmdArgInfo
{
    CmdArgKind kind;
    String name;
};

struct CmdArg
{
    int64_t i = 0;
    String s = "";
};

struct CommandInfo;

class Command
{
public:
    Command(const CommandInfo& info, const Vector<StringView>& args);

    int64_t get_arg_int(const StringView& name) const
    {
        return m_args.get(name).value().i;
    }

    String get_arg_string(const StringView& name) const
    {
        return m_args.get(name).value().s;
    }

private:
    HashMap<String, CmdArg> m_args;
};

typedef void (*CommandCallback)(const Command& cmd);

struct CommandInfo
{
    CommandCallback callback;
    Vector<CmdArgInfo> args;
};

#define CONSOLE_BUFFER_SIZE 128

class Console
{
public:
    Console();

    void register_command(const String& name, Vector<CmdArgInfo> args, CommandCallback callback);
    void exec();

    char *get_buffer()
    {
        return m_buffer;
    }

    size_t get_buffer_size() const
    {
        return CONSOLE_BUFFER_SIZE;
    }

private:
    HashMap<String, CommandInfo> m_commands;
    char m_buffer[CONSOLE_BUFFER_SIZE];
};

#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

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
    Command(const CommandInfo& info, const std::vector<String>& args);

    int64_t get_arg_int(const String& name) const
    {
        return m_args.at(name).i;
    }

    String get_arg_string(const String& name) const
    {
        return m_args.at(name).s;
    }

private:
    std::map<String, CmdArg> m_args;
};

typedef void (*CommandCallback)(const Command& cmd);

struct CommandInfo
{
    CommandCallback callback;
    std::vector<CmdArgInfo> args;
};

class Console
{
public:
    Console();

    void register_command(const String& name, std::vector<CmdArgInfo> args, CommandCallback callback);
    void exec();

    char *get_buffer()
    {
        return m_buffer.data();
    }

    size_t get_buffer_size() const
    {
        return m_buffer.size();
    }

private:
    std::map<String, CommandInfo> m_commands;
    std::array<char, 128> m_buffer;
};

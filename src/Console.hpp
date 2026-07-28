#pragma once

#include "stdext.hpp"

#include <cstdint>
#include <vector>

enum class CmdArgKind
{
    Int,
    String,
};

struct CmdArgInfo
{
    CmdArgKind kind;
    std::string name;
};

struct CmdArg
{
    int64_t i = 0;
    std::string s = "";
};

struct CommandInfo;

class Command
{
public:
    Command(const CommandInfo& info, const std::vector<std::string_view>& args);

    int64_t get_arg_int(std::string_view name) const
    {
        return m_args.find(name)->second.i;
    }

    std::string_view get_arg_string(std::string_view name) const
    {
        return m_args.find(name)->second.s;
    }

private:
    stdext::string_map<CmdArg> m_args;
};

typedef void (*CommandCallback)(const Command& cmd);

struct CommandInfo
{
    CommandCallback callback;
    std::vector<CmdArgInfo> args;
};

#define CONSOLE_BUFFER_SIZE 128

class Console
{
public:
    Console();

    void register_command(std::string_view name, std::vector<CmdArgInfo> args, CommandCallback callback);
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
    stdext::string_map<CommandInfo> m_commands;
    char m_buffer[CONSOLE_BUFFER_SIZE];
};

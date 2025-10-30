#pragma once

#include <map>

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
    Command(const CommandInfo& info, const std::vector<std::string>& args);

    int64_t get_arg_int(const std::string& name) const
    {
        return m_args.at(name).i;
    }

    std::string get_arg_string(const std::string& name) const
    {
        return m_args.at(name).s;
    }

private:
    std::map<std::string, CmdArg> m_args;
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

    void register_command(const std::string& name, std::vector<CmdArgInfo> args, CommandCallback callback);
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
    std::map<std::string, CommandInfo> m_commands;
    std::array<char, 128> m_buffer;
};

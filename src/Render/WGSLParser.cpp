#include "Render/WGSLParser.hpp"

#include <doctest/doctest.h>

#include <variant>

struct TokenIdent
{
    std::string value;
};

struct TokenOp
{
    char c;
};

struct TokenAttrib
{
    std::string name;
    std::optional<std::string> value;
};

using TokenVariant = std::variant<TokenIdent, TokenOp, TokenAttrib>;

template <>
struct std::formatter<TokenVariant> : std::formatter<std::string>
{
    auto format(const TokenVariant& token, std::format_context& ctx) const
    {
        std::string s;

        s += "Token(";

        if (std::holds_alternative<TokenIdent>(token))
        {
            const TokenIdent& ident = std::get<TokenIdent>(token);
            s += "Identifier, value=`" + ident.value + "`";
        }
        else if (std::holds_alternative<TokenOp>(token))
        {
            const TokenOp& op = std::get<TokenOp>(token);
            s += "Operator, value=`";
            s += op.c;
            s += "`";
        }
        else if (std::holds_alternative<TokenAttrib>(token))
        {
            const TokenAttrib& attrib = std::get<TokenAttrib>(token);
            s += "Attribute, name = `" + attrib.name + "`";
            if (attrib.value.has_value())
            {
                s += ", value = `" + attrib.value.value() + "`";
            }
        }

        s += ")";

        return std::format_to(ctx.out(), "{}", s);
    }
};

struct WGSLParser
{
    bool isop(char c)
    {
        return c == '(' || c == ')' || c == '{' || c == '}' || c == '<' || c == '>' || c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == ',' || c == ';' || c == ':';
    }

    std::vector<TokenVariant> tokenize(const std::string& source)
    {
        std::vector<TokenVariant> tokens;
        size_t index;

        for (index = 0; index < source.size(); index++)
        {
            // Skip whitespaces
            while (index < source.size() && std::isspace(source[index]))
                index++;

            char ch = source[index];

            if (ch == '@')
            {
                index++;

                std::string name;
                std::string value;

                while (index < source.size() && !std::isspace(source[index]) && source[index] != '(')
                    name += source[index++];

                while (index < source.size() && std::isspace(source[index]))
                    index++;

                if (source[index] == '(')
                {
                    index++;

                    while (index < source.size() && source[index] != ')')
                        value += source[index++];

                    index++;
                }

                TokenAttrib attrib;
                attrib.name = name;

                if (value.size() > 0)
                    attrib.value = value;

                tokens.push_back(attrib);
            }
            else if (isop(source[index]))
            {
                tokens.push_back(TokenOp{.c = source[index]});
            }
            else
            {
                std::string str;

                while (index < source.size() && !std::isspace(source[index]) && !isop(source[index]))
                    str += source[index++];

                if (isop(source[index]))
                    index--;

                if (str.size() == 0)
                    continue;

                tokens.push_back(TokenIdent{.value = str});
            }
        }

        return tokens;
    }

    WGSLModule parse(const std::string& source)
    {
        std::vector<TokenVariant> tokens = tokenize(source);
        WGSLModule module;

        // for (const auto& token : tokens)
        // {
        //     lib::println("{}", token);
        // }

        WGSLAttribs attribs;

        for (size_t index = 0; index < tokens.size(); index++)
        {
            const TokenVariant& token = tokens[index];

            if (std::holds_alternative<TokenAttrib>(token))
            {
                const TokenAttrib& attrib = std::get<TokenAttrib>(token);

                if (attrib.name == "builtin")
                    attribs.builtin = attrib.value;
                else if (attrib.name == "location")
                    attribs.location = std::stoi(attrib.value.value());
                else if (attrib.value == "group")
                    attribs.group = std::stoi(attrib.value.value());
                else if (attrib.value == "binding")
                    attribs.binding = std::stoi(attrib.value.value());
            }
        }

        return module;
    }
};

WGSLModule WGSLModule::parse(const std::string& source)
{
    return WGSLParser{}.parse(source);
}

TEST_CASE("WGSL parsing")
{
    WGSLModule module = WGSLModule::parse(R"(
        @group(0) @binding(0) var image: texture_2d_array<f32>;
        )");
}

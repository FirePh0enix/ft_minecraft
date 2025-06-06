#include "Render/WGSLParser.hpp"

enum class TokenKind : uint8_t
{
    Identifier,

    Struct, // `struct`
    Var,    // `var` with optional address space
    Fn,     // `fn`

    Attribute,

    Colon,       // `:`
    Semi,        // `;`
    Assign,      // `=`
    BracesLeft,  // `{`
    BracesRight, // `}`
    ParentLeft,  // `(`
    ParentRight, // `)`
};

union Token
{
    TokenKind kind;

    // @<name>(<value>) or @<name>
    struct
    {
        TokenKind kind;
        std::string name;
        std::optional<std::string> value;
    } attrib;
    // var< <address_space> > or var
    struct
    {
        TokenKind kind;
        std::string address_space;
    } var;

    ~Token()
    {
    }
};

struct WGSLParser
{
    std::vector<Token> tokenize(const std::string& source)
    {
        std::vector<Token> tokens;
        size_t index = 0;

        WGSLAttribs attribs;

        while (index < source.size())
        {
            // Skip whitespaces
            while (std::isspace(source[index]))
                index++;

            char ch = source[index];

            if (ch == '@')
            {
                index++;

                std::string name;
                std::string value;

                while (index < source.size() && !std::isspace(source[index]) && source[index] != '(')
                    name += source[index++];

                while (index < source.size() && !std::isspace(source[index]) && source[index] != '(')
                    name += source[index++];
            }

            index++;
        }

        return tokens;
    }

    WGSLModule parse(const std::string& source)
    {
        std::vector<Token> tokens = tokenize(source);
        WGSLModule module;

        return module;
    }
};

WGSLModule WGSLModule::parse(const std::string& source)
{
    return WGSLParser{}.parse(source);
}

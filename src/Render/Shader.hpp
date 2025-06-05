#pragma once

#include "Core/Class.hpp"
#include "Core/Ref.hpp"

class Shader : public Object
{
    CLASS(Shader2, Object);

public:
    enum class ErrorKind : uint8_t
    {
        FileNotFound,
        MissingDirective,
        RecursiveDirective,
        Compilation,
    };

    template <typename T>
    using Expected = std::expected<T, ErrorKind>;

    static Expected<Ref<Shader>> compile(const std::string& filename, const std::initializer_list<std::string>& definitions);

#ifdef __platform_web
    inline std::string get_code()
    {
        return m_code;
    }
#else
    inline std::vector<uint32_t> get_code()
    {
        return m_code;
    }
#endif

private:
#ifdef __platform_web
    std::string m_code;
#else
    std::vector<uint32_t> m_code;
#endif
};

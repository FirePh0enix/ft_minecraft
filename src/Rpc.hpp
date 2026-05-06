#pragma once

#include "Core/Containers/Vector.hpp"
#include "Core/String.hpp"
#include "Core/StringView.hpp"

#include <nlohmann/json.hpp>

#include <map>
#include <optional>
#include <variant>

enum class RpcTarget
{
    Client,
    Server,
    Both
};

enum class VariantType
{
    Bool,
    String,
};
using Variant = std::variant<std::monostate, bool, String>;

template <typename T>
inline VariantType get();

template <>
inline VariantType get<bool>() { return VariantType::Bool; }

template <>
inline VariantType get<String>() { return VariantType::Bool; }

inline void from_json(const nlohmann::json& j, Variant& m)
{
    m = j;
}
inline void to_json(nlohmann::json& j, const Variant& m)
{
    j = m;
}

using RpcFunc = std::function<void(void *, Vector<Variant> args)>;

struct Rpc
{
    RpcTarget target;
    Vector<VariantType> args;
    RpcFunc func;
};

class RpcDispatch
{
public:
    RpcDispatch()
    {
    }

    template <typename E, typename... Args>
    void add(String name, RpcFunc f, RpcTarget target = RpcTarget::Both)
    {
        Vector<VariantType> arguments;
        (arguments.append(get<Args>()), ...);

        m_functions[name] = Rpc{.target = target, .args = arguments, .func = f};
    }

    std::optional<Rpc> get(const StringView& name)
    {
        if (m_functions.contains(name))
            return m_functions.at(name);
        return std::nullopt;
    }

private:
    std::map<String, Rpc> m_functions;
};

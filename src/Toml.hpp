#pragma once

#define TOML_COMPILER_HAS_EXCEPTIONS 0
#include <toml++/toml.hpp>

inline void toml_merge_left(toml::table& lhs, toml::table&& rhs);

inline void toml_merge_left(toml::array& lhs, toml::array&& rhs)
{
    rhs.for_each(
        [&](std::size_t index, auto&& rhs_val)
        {
            // rhs index not found in lhs - direct move
            if (lhs.size() <= index)
            {
                lhs.push_back(std::move(rhs_val));
                return;
            }

            // both elements were the same container type -  recurse into them
            if constexpr (toml::is_container<decltype(rhs_val)>)
            {
                using rhs_type = std::remove_cv_t<std::remove_reference_t<decltype(rhs_val)>>;
                if (auto lhs_child = lhs[index].as<rhs_type>())
                {
                    toml_merge_left(*lhs_child, std::move(rhs_val));
                    return;
                }
            }

            // replace lhs element with rhs
            lhs.replace(lhs.cbegin() + static_cast<std::ptrdiff_t>(index), std::move(rhs_val));
        });
}

inline void toml_merge_left(toml::table& lhs, toml::table&& rhs)
{
    rhs.for_each(
        [&](const toml::key& rhs_key, auto&& rhs_val)
        {
            auto lhs_it = lhs.lower_bound(rhs_key);

            // rhs key not found in lhs - direct move
            if (lhs_it == lhs.cend() || lhs_it->first != rhs_key)
            {
                using rhs_type = std::remove_cv_t<std::remove_reference_t<decltype(rhs_val)>>;
                lhs.emplace_hint<rhs_type>(lhs_it, rhs_key, std::move(rhs_val));
                return;
            }

            // both children were the same container type -  recurse into them
            if constexpr (toml::is_container<decltype(rhs_val)>)
            {
                using rhs_type = std::remove_cv_t<std::remove_reference_t<decltype(rhs_val)>>;
                if (auto lhs_child = lhs_it->second.as<rhs_type>())
                {
                    toml_merge_left(*lhs_child, std::move(rhs_val));
                    return;
                }
            }

            // replace lhs value with rhs
            lhs.insert_or_assign(rhs_key, std::move(rhs_val));
        });
}

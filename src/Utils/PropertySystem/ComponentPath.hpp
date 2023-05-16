#pragma once

#include "src/Utils/CStringView.hpp"

#include <string>
#include <string_view>

namespace osc
{
    // COMPONENT PATH
    //
    // - a normalized (i.e. ../x/.. --> ..) path string
    // - that encodes a path from a source component to a destination
    //   component (e.g. ../to/destination)
    // - where the path may be "absolute", which is a special encoding
    //   that tells the implementation that the source component must
    //   be the root of the component tree (e.g. /path/from/root/to/destination)
    class ComponentPath final {
    public:
        [[nodiscard]] static inline constexpr char delimiter() noexcept
        {
            return '/';
        }

        explicit ComponentPath(std::string_view);

        operator CStringView () const noexcept
        {
            return m_NormalizedPath;
        }

        operator std::string_view () const noexcept
        {
            return m_NormalizedPath;
        }
    private:
        std::string m_NormalizedPath;
    };

    bool IsAbsolute(ComponentPath const&) noexcept;
}
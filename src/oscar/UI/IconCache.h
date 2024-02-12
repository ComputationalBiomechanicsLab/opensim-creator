#pragma once

#include <oscar/Platform/ResourceLoader.h>

#include <memory>
#include <string_view>

namespace osc { class Icon; }

namespace osc
{
    class IconCache final {
    public:
        IconCache(
            ResourceLoader loaderPrefixedAtDirContainingSVGs,
            float verticalScale
        );
        IconCache(IconCache const&) = delete;
        IconCache(IconCache&&) noexcept;
        IconCache& operator=(IconCache const&) = delete;
        IconCache& operator=(IconCache&&) noexcept;
        ~IconCache() noexcept;

        Icon const& getIcon(std::string_view iconName) const;

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

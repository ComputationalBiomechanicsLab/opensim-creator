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
        IconCache(const IconCache&) = delete;
        IconCache(IconCache&&) noexcept;
        IconCache& operator=(const IconCache&) = delete;
        IconCache& operator=(IconCache&&) noexcept;
        ~IconCache() noexcept;

        const Icon& getIcon(std::string_view iconName) const;

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

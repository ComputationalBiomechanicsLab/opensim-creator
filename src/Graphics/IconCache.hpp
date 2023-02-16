#pragma once

#include <filesystem>
#include <memory>
#include <string_view>

namespace osc { class Icon; }

namespace osc
{
    class IconCache final {
    public:
        IconCache(std::filesystem::path const& iconsDir, float verticalScale);
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
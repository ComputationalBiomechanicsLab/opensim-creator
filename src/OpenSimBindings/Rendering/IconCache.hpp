#pragma once

#include <memory>
#include <string_view>

namespace osc { class Config; }
namespace osc { class Icon; }

namespace osc
{
    class IconCache final {
    public:
        IconCache();
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
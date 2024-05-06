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
            ResourceLoader loader_prefixed_at_dir_containing_svgs,
            float vertical_scale
        );
        IconCache(const IconCache&) = delete;
        IconCache(IconCache&&) noexcept;
        IconCache& operator=(const IconCache&) = delete;
        IconCache& operator=(IconCache&&) noexcept;
        ~IconCache() noexcept;

        const Icon& find_or_throw(std::string_view icon_name) const;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}

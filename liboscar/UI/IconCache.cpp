#include "IconCache.h"

#include <liboscar/Formats/SVG.h>
#include <liboscar/Platform/ResourceLoader.h>
#include <liboscar/UI/Icon.h>
#include <liboscar/Utils/Assertions.h>
#include <liboscar/Utils/TransparentStringHasher.h>

#include <ankerl/unordered_dense.h>

#include <memory>
#include <stdexcept>
#include <sstream>
#include <string>
#include <string_view>

using osc::Icon;

class osc::IconCache::Impl final {
public:
    explicit Impl(
        ResourceLoader& loader_prefixed_at_dir_containing_svgs,
        float vertical_scale,
        float device_pixel_ratio)
    {
        OSC_ASSERT(vertical_scale > 0.0f && "icon cache's vertical scale must be a positive number");
        OSC_ASSERT(device_pixel_ratio > 0.0f && "icon cache's device pixel ratio must be a positive number");

        const auto it = loader_prefixed_at_dir_containing_svgs.iterate_directory(".");

        for (auto el = it(); el; el = it()) {
            const ResourcePath& p = *el;

            if (p.has_extension(".svg")) {
                Texture2D texture = load_texture2D_from_svg(
                    loader_prefixed_at_dir_containing_svgs.open(p),
                    vertical_scale,
                    device_pixel_ratio
                );

                icons_by_name_.try_emplace(
                    p.stem(),
                    std::move(texture),
                    Rect{{0.0f, 1.0f}, {1.0f, 0.0f}}
                );
            }
        }
    }

    const Icon& find_or_throw(std::string_view icon_name) const
    {
        if (const auto it = icons_by_name_.find(icon_name); it != icons_by_name_.end()) {
            return it->second;
        }
        else {
            std::stringstream ss;
            ss << "error finding icon: cannot find: " << icon_name;
            throw std::runtime_error{std::move(ss).str()};
        }
    }

private:
    ankerl::unordered_dense::map<std::string, Icon, TransparentStringHasher, std::equal_to<>> icons_by_name_;
};


osc::IconCache::IconCache(ResourceLoader loader_prefixed_at_dir_containing_svgs, float vertical_scale, float device_pixel_ratio) :
    impl_{std::make_unique<Impl>(loader_prefixed_at_dir_containing_svgs, vertical_scale, device_pixel_ratio)}
{}
osc::IconCache::IconCache(IconCache&&) noexcept = default;
osc::IconCache& osc::IconCache::operator=(IconCache&&) noexcept = default;
osc::IconCache::~IconCache() noexcept = default;

const Icon& osc::IconCache::find_or_throw(std::string_view icon_name) const
{
    return impl_->find_or_throw(icon_name);
}

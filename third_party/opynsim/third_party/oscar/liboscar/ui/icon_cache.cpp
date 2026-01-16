#include "icon_cache.h"

#include <liboscar/formats/svg.h>
#include <liboscar/platform/resource_loader.h>
#include <liboscar/ui/icon.h>
#include <liboscar/utils/assertions.h>
#include <liboscar/utils/transparent_string_hasher.h>

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

        for (const ResourcePath& p : loader_prefixed_at_dir_containing_svgs.iterate_directory(".")) {
            if (not p.has_extension(".svg")) {
                continue;  // Skip non-SVGs
            }
            // Else: read the SVG into a texture and add it to the lookup

            Texture2D texture = SVG::read_into_texture(
                loader_prefixed_at_dir_containing_svgs.open(p),
                vertical_scale,
                device_pixel_ratio
            );

            icons_by_name_.try_emplace(
                p.stem(),
                std::move(texture),
                Rect::from_corners({0.0f, 1.0f}, {1.0f, 0.0f})
            );
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

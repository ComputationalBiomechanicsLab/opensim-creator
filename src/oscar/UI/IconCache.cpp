#include "IconCache.h"

#include <oscar/Formats/SVG.h>
#include <oscar/Platform/ResourceLoader.h>
#include <oscar/UI/Icon.h>

#include <memory>
#include <stdexcept>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

using osc::Icon;

class osc::IconCache::Impl final {
public:
    Impl(ResourceLoader& loaderPrefixedAtDirContainingSVGs, float verticalScale)
    {
        auto it = loaderPrefixedAtDirContainingSVGs.iterateDirectory(".");

        for (auto el = it(); el; el = it()) {
            ResourcePath const& p = *el;

            if (p.hasExtension(".svg"))
            {
                Texture2D texture = ReadSVGIntoTexture(
                    loaderPrefixedAtDirContainingSVGs.open(p),
                    verticalScale
                );
                texture.setFilterMode(TextureFilterMode::Nearest);

                m_Icons.try_emplace(
                    p.stem(),
                    std::move(texture),
                    Rect{{0.0f, 1.0f}, {1.0f, 0.0f}}
                );
            }
        }
    }

    Icon const& getIcon(std::string_view iconName) const
    {
        if (auto const it = m_Icons.find(std::string{iconName}); it != m_Icons.end())
        {
            return it->second;
        }
        else
        {
            std::stringstream ss;
            ss << "error finding icon: cannot find: " << iconName;
            throw std::runtime_error{std::move(ss).str()};
        }
    }

private:
    std::unordered_map<std::string, Icon> m_Icons;
};


// public API (PIMPL)

osc::IconCache::IconCache(ResourceLoader loaderPrefixedAtDirContainingSVGs, float verticalScale) :
    m_Impl{std::make_unique<Impl>(loaderPrefixedAtDirContainingSVGs, verticalScale)}
{
}
osc::IconCache::IconCache(IconCache&&) noexcept = default;
osc::IconCache& osc::IconCache::operator=(IconCache&&) noexcept = default;
osc::IconCache::~IconCache() noexcept = default;

Icon const& osc::IconCache::getIcon(std::string_view iconName) const
{
    return m_Impl->getIcon(iconName);
}

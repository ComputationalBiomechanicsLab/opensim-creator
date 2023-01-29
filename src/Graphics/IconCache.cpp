#include "IconCache.hpp"

#include "src/Graphics/Icon.hpp"
#include "src/Formats/SVG.hpp"

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>


class osc::IconCache::Impl final {
public:
    Impl(std::filesystem::path const& iconsDir, float verticalScale)
    {
        // iterate through the icons dir and load each SVG file as an icon
        std::filesystem::directory_iterator directoryIterator{iconsDir};
        for (std::filesystem::path const& p : directoryIterator)
        {
            if (p.extension() == ".svg")
            {
                Texture2D texture = LoadTextureFromSVGFile(p, verticalScale);
                texture.setFilterMode(TextureFilterMode::Nearest);
                m_Icons.try_emplace(p.stem().string(), std::move(texture), glm::vec2{0.0f, 1.0f}, glm::vec2{1.0f, 0.0f});
            }
        }
    }

    Icon const& getIcon(std::string_view iconName) const
    {
        auto const it = m_Icons.find(std::string{iconName});
        if (it != m_Icons.end())
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

osc::IconCache::IconCache(std::filesystem::path const& iconsDir, float verticalScale) :
    m_Impl{std::make_unique<Impl>(iconsDir, verticalScale)}
{
}
osc::IconCache::IconCache(IconCache&&) noexcept = default;
osc::IconCache& osc::IconCache::operator=(IconCache&&) noexcept = default;
osc::IconCache::~IconCache() noexcept = default;

osc::Icon const& osc::IconCache::getIcon(std::string_view iconName) const
{
    return m_Impl->getIcon(std::move(iconName));
}
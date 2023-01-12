#include "IconCache.hpp"

#include "src/Formats/SVG.hpp"
#include "src/OpenSimBindings/Icon.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Config.hpp"

#include <imgui.h>

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>


class osc::IconCache::Impl final {
public:
    Impl()
    {
        // iterate through the `icons/` resource dir and load each SVG icon
        Config const& config = osc::App::config();

        std::filesystem::path const iconsDir = config.getResourceDir() / "icons";
        std::filesystem::directory_iterator directoryIterator{iconsDir};
        for (std::filesystem::path const& p : directoryIterator)
        {
            if (p.extension() == ".svg")
            {
                // normal size
                {
                    Texture2D texture = LoadTextureFromSVGFile(p, ImGui::GetTextLineHeight()/128.0f);
                    texture.setFilterMode(TextureFilterMode::Mipmap);
                    m_Icons.try_emplace(p.stem().string(), std::move(texture), glm::vec2{0.0f, 1.0f}, glm::vec2{1.0f, 0.0f});
                }

                // larger
                {
                    Texture2D texture = LoadTextureFromSVGFile(p, 1.5f*ImGui::GetTextLineHeight()/128.0f);
                    texture.setFilterMode(TextureFilterMode::Mipmap);
                    m_LargerIcons.try_emplace(p.stem().string(), std::move(texture), glm::vec2{0.0f, 1.0f}, glm::vec2{1.0f, 0.0f});
                }
            }
        }
    }

    Icon const& getIcon(std::string_view iconName) const
    {
        return getIcon(m_Icons, std::move(iconName));
    }

    Icon const& getIconLarger(std::string_view iconName) const
    {
        return getIcon(m_LargerIcons, std::move(iconName));
    }

private:


    Icon const& getIcon(std::unordered_map<std::string, Icon> const& storage, std::string_view iconName) const
    {
        auto const it = storage.find(std::string{iconName});
        if (it != storage.end())
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

    std::unordered_map<std::string, Icon> m_Icons;
    std::unordered_map<std::string, Icon> m_LargerIcons;
};


// public API (PIMPL)

osc::IconCache::IconCache() :
    m_Impl{std::make_unique<Impl>()}
{
}
osc::IconCache::IconCache(IconCache&&) noexcept = default;
osc::IconCache& osc::IconCache::operator=(IconCache&&) noexcept = default;
osc::IconCache::~IconCache() noexcept = default;

osc::Icon const& osc::IconCache::getIcon(std::string_view iconName) const
{
    return m_Impl->getIcon(std::move(iconName));
}

osc::Icon const& osc::IconCache::getIconLarger(std::string_view iconName) const
{
    return m_Impl->getIconLarger(std::move(iconName));
}
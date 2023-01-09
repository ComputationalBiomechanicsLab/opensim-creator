#include "SVG.hpp"

#include "src/Platform/App.hpp"
#include "src/Utils/Assertions.hpp"

#include <glm/vec2.hpp>
#include <lunasvg.h>
#include <nonstd/span.hpp>

#include <cstdint>
#include <filesystem>
#include <memory>

osc::Texture2D osc::LoadTextureFromSVGFile(std::filesystem::path const& p)
{
    std::unique_ptr<lunasvg::Document> doc = lunasvg::Document::loadFromFile(p.string());
    OSC_ASSERT(doc != nullptr && "error loading SVG document");
    lunasvg::Matrix m{1.0, 0.0, 0.0, -1.0, 0.0, doc->height()};  // column major: flip Y
    doc->setMatrix(m);
    lunasvg::Bitmap bitmap = doc->renderToBitmap(static_cast<uint32_t>(doc->width()), static_cast<uint32_t>(doc->height()), 0x00000000);
    bitmap.convertToRGBA();

    osc::Texture2D t(glm::ivec2(doc->width(), doc->height()), {bitmap.data(), bitmap.width()*bitmap.height()*4}, 4);
    t.setWrapMode(osc::TextureWrapMode::Clamp);
    t.setFilterMode(osc::TextureFilterMode::Nearest);

    return t;
}

osc::Texture2D osc::LoadTextureFromSVGResource(std::string_view resourceName)
{
    return LoadTextureFromSVGFile(osc::App::resource(resourceName));
}
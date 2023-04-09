#include "SVG.hpp"

#include "src/Utils/Assertions.hpp"

#include <glm/vec2.hpp>
#include <lunasvg/lunasvg.h>
#include <nonstd/span.hpp>

#include <cstdint>
#include <filesystem>
#include <memory>

osc::Texture2D osc::LoadTextureFromSVGFile(std::filesystem::path const& p, float scale)
{
    // load the SVG document
    std::unique_ptr<lunasvg::Document> doc = lunasvg::Document::loadFromFile(p.string());
    OSC_ASSERT(doc != nullptr && "error loading SVG document");

    // when rendering the document's contents, flip Y so that it's compatible with the
    // renderer's coordinate system
    lunasvg::Matrix m{1.0, 0.0, 0.0, -1.0, 0.0, doc->height()};
    doc->setMatrix(m);

    // render to a rescaled bitmap
    glm::vec<2, uint32_t> const bitmapDimensions(static_cast<uint32_t>(scale*doc->width()), static_cast<uint32_t>(scale*doc->height()));
    lunasvg::Bitmap bitmap = doc->renderToBitmap(bitmapDimensions.x, bitmapDimensions.y, 0x00000000);
    bitmap.convertToRGBA();

    osc::Texture2D t({bitmap.width(), bitmap.height()}, TextureFormat::RGBA32, {bitmap.data(), bitmap.width()*bitmap.height()*4});
    t.setWrapMode(osc::TextureWrapMode::Clamp);
    t.setFilterMode(osc::TextureFilterMode::Nearest);

    return t;
}

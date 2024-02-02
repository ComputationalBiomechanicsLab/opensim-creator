#include "SVG.hpp"

#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Utils/Assertions.hpp>

#include <lunasvg.h>

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <memory>
#include <span>
#include <sstream>
#include <stdexcept>

using osc::Texture2D;

Texture2D osc::ReadSVGIntoTexture(
    std::istream& inputStream,
    float scale)
{
    // read SVG content into a std::string and parse it
    std::string data;
    std::copy(
        std::istreambuf_iterator{inputStream},
        std::istreambuf_iterator<std::istream::char_type>{},
        std::back_inserter(data)
    );

    // parse data into SVG document
    std::unique_ptr<lunasvg::Document> doc = lunasvg::Document::loadFromData(data);
    OSC_ASSERT(doc != nullptr && "error loading SVG document");

    // when rendering the document's contents, flip Y so that it's compatible with the
    // renderer's coordinate system
    lunasvg::Matrix m{1.0, 0.0, 0.0, -1.0, 0.0, doc->height()};
    doc->setMatrix(m);

    // render to a rescaled bitmap
    Vec2u32 const bitmapDimensions
    {
        static_cast<uint32_t>(scale*doc->width()),
        static_cast<uint32_t>(scale*doc->height())
    };
    lunasvg::Bitmap bitmap = doc->renderToBitmap(bitmapDimensions.x, bitmapDimensions.y, 0x00000000);
    bitmap.convertToRGBA();

    // return as a GPU-ready texture
    Texture2D rv
    {
        {bitmap.width(), bitmap.height()},
        TextureFormat::RGBA32,
        ColorSpace::sRGB,
        TextureWrapMode::Clamp,
        TextureFilterMode::Nearest,
    };
    rv.setPixelData({bitmap.data(), static_cast<size_t>(bitmap.width()*bitmap.height()*4)});
    return rv;
}

Texture2D osc::LoadTextureFromSVGFile(
    std::filesystem::path const& path,
    float scale)
{
    std::ifstream f{path};
    if (!f)
    {
        std::stringstream ss;
        ss << path << ": failed to load input file";
        throw std::runtime_error{std::move(ss).str()};
    }
    return ReadSVGIntoTexture(f, scale);
}

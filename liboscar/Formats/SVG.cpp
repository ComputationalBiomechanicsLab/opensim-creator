#include "SVG.h"

#include <liboscar/Graphics/ColorSpace.h>
#include <liboscar/Maths/Vec2.h>
#include <liboscar/Utils/Assertions.h>

#include <lunasvg.h>

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <memory>
#include <span>
#include <sstream>

using namespace osc;

Texture2D osc::load_texture2D_from_svg(std::istream& in, float scale)
{
    // read SVG content into a `std::string`
    std::string data;
    std::copy(
        std::istreambuf_iterator{in},
        std::istreambuf_iterator<std::istream::char_type>{},
        std::back_inserter(data)
    );

    // parse the `std::string` as an SVG document
    const std::unique_ptr<lunasvg::Document> svg_document = lunasvg::Document::loadFromData(data);
    OSC_ASSERT(svg_document != nullptr && "error loading SVG document");

    // when rendering the document's contents, flip Y so that it's compatible with the
    // renderer's coordinate system
    const lunasvg::Matrix m{1.0, 0.0, 0.0, -1.0, 0.0, svg_document->height()};
    svg_document->setMatrix(m);

    // render to a rescaled bitmap
    const Vec2u32 bitmap_dimensions{
        static_cast<uint32_t>(scale*svg_document->width()),
        static_cast<uint32_t>(scale*svg_document->height())
    };
    lunasvg::Bitmap bitmap = svg_document->renderToBitmap(bitmap_dimensions.x, bitmap_dimensions.y, 0x00000000);
    bitmap.convertToRGBA();

    // return as a GPU-ready texture
    Texture2D rv{
        {bitmap.width(), bitmap.height()},
        TextureFormat::RGBA32,
        ColorSpace::sRGB,
        TextureWrapMode::Clamp,
        TextureFilterMode::Nearest,
    };
    rv.set_pixel_data({bitmap.data(), static_cast<size_t>(bitmap.width()*bitmap.height()*4)});
    return rv;
}

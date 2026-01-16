#include "svg.h"

#include <liboscar/graphics/color_space.h>
#include <liboscar/maths/vector2.h>
#include <liboscar/utils/assertions.h>

#include <lunasvg.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <memory>
#include <span>
#include <sstream>

using namespace osc;

Texture2D osc::SVG::read_into_texture(std::istream& in, float scale, float device_pixel_ratio)
{
    OSC_ASSERT_ALWAYS(scale > 0.0f && "svg scale factor must be greater than zero");
    OSC_ASSERT_ALWAYS(device_pixel_ratio > 0.0f && "device pixel ratio must be greater than zero");

    // read SVG content into a `std::string`
    std::string data;
    std::copy(
        std::istreambuf_iterator{in},
        std::istreambuf_iterator<std::istream::char_type>{},
        std::back_inserter(data)
    );

    // parse the `std::string` as an SVG document
    const std::unique_ptr<lunasvg::Document> svg_document = lunasvg::Document::loadFromData(data);
    OSC_ASSERT_ALWAYS(svg_document != nullptr && "error loading SVG document");

    // when rendering the document's contents, flip Y so that Y=0 represents the bottom of the image
    // and Y=H represents the top (i.e. a right-handed coordinate system that matches `Texture2D`).
    const float pixel_scale = scale * device_pixel_ratio;
    const lunasvg::Matrix transform{pixel_scale, 0.0f, 0.0f, -pixel_scale, 0.0f, std::ceil(pixel_scale*svg_document->height())};

    // create a `lunasvg::Bitmap` that lunasvg can render into
    lunasvg::Bitmap bitmap{
        static_cast<int>(std::ceil(pixel_scale*svg_document->width())),
        static_cast<int>(std::ceil(pixel_scale*svg_document->height())),
    };
    bitmap.clear(0x00000000);  // ensure the bitmap is cleared before rendering
    svg_document->render(bitmap, transform);
    bitmap.convertToRGBA();  // convert lunasvg's premultiplied ARGB32 to unassociated RGBA

    // return as a GPU-ready texture
    Texture2D rv{
        {bitmap.width(), bitmap.height()},
        TextureFormat::RGBA32,
        ColorSpace::sRGB,
        TextureWrapMode::Clamp,
        TextureFilterMode::Nearest,
    };
    rv.set_pixel_data({bitmap.data(), static_cast<size_t>(bitmap.width()*bitmap.height()*4)});
    rv.set_device_pixel_ratio(device_pixel_ratio);
    return rv;
}

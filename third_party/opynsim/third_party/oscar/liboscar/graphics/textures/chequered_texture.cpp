#include "chequered_texture.h"

#include <liboscar/graphics/color32.h>
#include <liboscar/graphics/color_space.h>
#include <liboscar/graphics/texture2_d.h>
#include <liboscar/graphics/texture_format.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/vector2.h>
#include <liboscar/utils/object_representation.h>

#include <vector>

using namespace osc;

namespace
{
    Texture2D generate_chequer_texture()
    {
        constexpr Vector2i chequer_pixel_dimensions = {1, 1};
        constexpr Vector2i texture_pixel_dimensions = 2 * chequer_pixel_dimensions;
        constexpr Color32 on_color = Color32::white();
        constexpr Color32 off_color = Color32::lightest_grey();

        std::vector<Color32> pixels;
        pixels.reserve(area_of(texture_pixel_dimensions));
        for (int y = 0; y < texture_pixel_dimensions.y(); ++y) {
            const bool y_on = (y / chequer_pixel_dimensions.y()) % 2 == 0;
            for (int x = 0; x < texture_pixel_dimensions.x(); ++x) {
                const bool x_on = (x / chequer_pixel_dimensions.x()) % 2 == 0;
                pixels.push_back(y_on ^ x_on ? on_color : off_color);
            }
        }

        Texture2D rv{
            texture_pixel_dimensions,
            TextureFormat::RGBA32,
            ColorSpace::sRGB,
            TextureWrapMode::Repeat,
            TextureFilterMode::Nearest,
        };
        rv.set_pixel_data(view_object_representations<uint8_t>(pixels));
        return rv;
    }
}

osc::ChequeredTexture::ChequeredTexture() :
    texture_{generate_chequer_texture()}
{}

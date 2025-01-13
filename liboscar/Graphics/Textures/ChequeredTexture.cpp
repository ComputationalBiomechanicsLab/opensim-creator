#include "ChequeredTexture.h"

#include <liboscar/Graphics/Color32.h>
#include <liboscar/Graphics/ColorSpace.h>
#include <liboscar/Graphics/Texture2D.h>
#include <liboscar/Graphics/TextureFormat.h>
#include <liboscar/Maths/MathHelpers.h>
#include <liboscar/Maths/Vec2.h>
#include <liboscar/Utils/ObjectRepresentation.h>

#include <vector>

using namespace osc;

namespace
{
    Texture2D generate_chequer_texture()
    {
        constexpr Vec2i chequer_dims = {1, 1};
        constexpr Vec2i texture_dims = 2 * chequer_dims;
        constexpr Color32 on_color = Color32::white();
        constexpr Color32 off_color = Color32::very_light_grey();

        std::vector<Color32> pixels;
        pixels.reserve(area_of(texture_dims));
        for (int y = 0; y < texture_dims.y; ++y) {
            const bool y_on = (y / chequer_dims.y) % 2 == 0;
            for (int x = 0; x < texture_dims.x; ++x) {
                const bool x_on = (x / chequer_dims.x) % 2 == 0;
                pixels.push_back(y_on ^ x_on ? on_color : off_color);
            }
        }

        Texture2D rv{
            texture_dims,
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

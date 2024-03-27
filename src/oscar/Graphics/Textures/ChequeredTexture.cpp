

#include "ChequeredTexture.h"

#include <oscar/Graphics/Color32.h>
#include <oscar/Graphics/ColorSpace.h>
#include <oscar/Graphics/Texture2D.h>
#include <oscar/Graphics/TextureFormat.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Utils/ObjectRepresentation.h>

#include <vector>

using namespace osc;

namespace
{
    Texture2D generateChequeredTexture()
    {
        constexpr Vec2i chequerDims = {32, 32};
        constexpr Vec2i textureDims = 2 * chequerDims;
        constexpr Color32 onColor = {0xff, 0xff, 0xff, 0xff};
        constexpr Color32 offColor = {0xf3, 0xf3, 0xf3, 0xff};

        std::vector<Color32> pixels;
        pixels.reserve(area(textureDims));
        for (int y = 0; y < textureDims.y; ++y) {
            const bool yOn = (y / chequerDims.y) % 2 == 0;
            for (int x = 0; x < textureDims.x; ++x) {
                const bool xOn = (x / chequerDims.x) % 2 == 0;
                pixels.push_back(yOn ^ xOn ? onColor : offColor);
            }
        }

        Texture2D rv{
            textureDims,
            TextureFormat::RGBA32,
            ColorSpace::sRGB,
            TextureWrapMode::Repeat,
            TextureFilterMode::Mipmap,
        };
        rv.setPixelData(ViewObjectRepresentations<uint8_t>(pixels));
        return rv;
    }
}

osc::ChequeredTexture::ChequeredTexture() :
    m_Texture{generateChequeredTexture()}
{}

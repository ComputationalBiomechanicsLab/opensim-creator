#pragma once

#include <oscar/Graphics/Texture2D.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Vec2.h>

#include <utility>

namespace osc
{
    class Icon final {
    public:
        Icon(
            Texture2D texture_,
            Rect const& textureCoordinates_) :

            m_Texture{std::move(texture_)},
            m_TextureCoordinates{textureCoordinates_}
        {
        }

        Texture2D const& getTexture() const
        {
            return m_Texture;
        }

        Vec2i getDimensions() const
        {
            return m_Texture.getDimensions();
        }

        Rect const& getTextureCoordinates() const
        {
            return m_TextureCoordinates;
        }

    private:
        Texture2D m_Texture;
        Rect m_TextureCoordinates;
    };
}

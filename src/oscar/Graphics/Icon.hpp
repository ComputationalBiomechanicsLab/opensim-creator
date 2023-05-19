#pragma once

#include "oscar/Graphics/Texture2D.hpp"

#include <glm/vec2.hpp>

#include <utility>

namespace osc
{
    class Icon final {
    public:
        Icon(
            Texture2D texture_,
            glm::vec2 topLeftTextureCoord_,
            glm::vec2 bottomRightTextureCoord_) :

            m_Texture{std::move(texture_)},
            m_TopLeftTextureCoord{topLeftTextureCoord_},
            m_BottomRightTextureCoord{bottomRightTextureCoord_}
        {
        }

        Texture2D const& getTexture() const
        {
            return m_Texture;
        }

        glm::ivec2 getDimensions() const
        {
            return m_Texture.getDimensions();
        }

        glm::vec2 getTopLeftTextureCoord() const
        {
            return m_TopLeftTextureCoord;
        }

        glm::vec2 getBottomRightTextureCoord() const
        {
            return m_BottomRightTextureCoord;
        }

    private:
        Texture2D m_Texture;
        glm::vec2 m_TopLeftTextureCoord;
        glm::vec2 m_BottomRightTextureCoord;
    };
}
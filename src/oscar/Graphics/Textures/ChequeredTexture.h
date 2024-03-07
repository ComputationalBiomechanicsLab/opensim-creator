#pragma once

#include <oscar/Graphics/Texture2D.h>

namespace osc
{
    class ChequeredTexture final {
    public:
        ChequeredTexture();

        Texture2D const& texture() const { return m_Texture; }
        operator Texture2D const& () const { return m_Texture; }
    private:
        Texture2D m_Texture;
    };
}

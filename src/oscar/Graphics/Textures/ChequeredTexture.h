#pragma once

#include <oscar/Graphics/Texture2D.h>

namespace osc
{
    class ChequeredTexture final {
    public:
        ChequeredTexture();

        const Texture2D& texture() const { return m_Texture; }
        operator const Texture2D& () const { return m_Texture; }
    private:
        Texture2D m_Texture;
    };
}

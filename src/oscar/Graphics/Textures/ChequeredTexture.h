#pragma once

#include <oscar/Graphics/Texture2D.h>

namespace osc
{
    class ChequeredTexture final {
    public:
        ChequeredTexture();

        const Texture2D& texture() const { return texture_; }
        operator const Texture2D& () const { return texture_; }
    private:
        Texture2D texture_;
    };
}

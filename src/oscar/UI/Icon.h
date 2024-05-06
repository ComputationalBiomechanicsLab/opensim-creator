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
            Texture2D texture,
            const Rect& texture_coordinates) :

            texture_{std::move(texture)},
            texture_coordinates_{texture_coordinates}
        {}

        const Texture2D& texture() const { return texture_; }

        Vec2i dimensions() const { return texture_.dimensions(); }

        const Rect& texture_coordinates() const { return texture_coordinates_; }

    private:
        Texture2D texture_;
        Rect texture_coordinates_;
    };
}

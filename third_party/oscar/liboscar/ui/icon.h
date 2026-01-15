#pragma once

#include <liboscar/graphics/texture2_d.h>
#include <liboscar/maths/rect.h>
#include <liboscar/maths/vector2.h>

#include <utility>

namespace osc
{
    class Icon final {
    public:
        explicit Icon(
            Texture2D texture,
            const Rect& texture_coordinates) :

            texture_{std::move(texture)},
            texture_coordinates_{texture_coordinates}
        {}

        const Texture2D& texture() const { return texture_; }

        Vector2 dimensions() const { return texture_.dimensions(); }

        const Rect& texture_coordinates() const { return texture_coordinates_; }

    private:
        Texture2D texture_;
        Rect texture_coordinates_;
    };
}

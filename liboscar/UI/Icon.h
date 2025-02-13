#pragma once

#include <liboscar/Graphics/Texture2D.h>
#include <liboscar/Maths/Rect.h>
#include <liboscar/Maths/Vec2.h>

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

        Vec2 dimensions() const { return texture_.device_independent_dimensions(); }

        const Rect& texture_coordinates() const { return texture_coordinates_; }

    private:
        Texture2D texture_;
        Rect texture_coordinates_;
    };
}

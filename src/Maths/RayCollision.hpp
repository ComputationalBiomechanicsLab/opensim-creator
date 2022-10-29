#pragma once

namespace osc
{
    struct RayCollision final {
        bool hit;
        float distance;

        explicit operator bool () const
        {
            return hit;
        }
    };
}

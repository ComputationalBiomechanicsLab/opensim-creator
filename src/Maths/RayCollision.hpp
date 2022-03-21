#pragma once

namespace osc
{
    struct RayCollision final {
        bool hit;
        float distance;

        operator bool () {
            return hit;
        }
    };
}

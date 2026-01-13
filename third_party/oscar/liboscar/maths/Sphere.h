#pragma once

#include <liboscar/maths/Vector3.h>

#include <iosfwd>

namespace osc
{
    struct Sphere final {
        Vector3 origin{};
        float radius{1.0f};
    };

    std::ostream& operator<<(std::ostream&, const Sphere&);
}

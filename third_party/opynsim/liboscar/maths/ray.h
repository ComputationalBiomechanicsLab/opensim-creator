#pragma once

#include <liboscar/maths/constants.h>
#include <liboscar/maths/vector.h>

#include <iosfwd>

namespace osc { template<typename T, size_t C, size_t R> struct Matrix; }
namespace osc { struct Transform; }

namespace osc
{
    // an infinitely long object with no width with an origin position and direction in 3D space
    //
    // - see `LineSegment` for the finite version of this
    // - sometimes called `Ray` in the literature
    struct Ray final {
        friend bool operator==(const Ray&, const Ray&) = default;

        Vector3 origin{};
        Vector3 direction = {0.0f, 1.0f, 0.0f};
    };

    std::ostream& operator<<(std::ostream&, const Ray&);

    // Returns `true` if `a` and `b` lie on the same mathematical line
    // and point in the same direction to within some (dot-product-defined)
    // tolerance.
    bool is_colinear_and_codirectional(
        const Ray& a,
        const Ray& b,
        float tolerance = epsilon_v<float>
    );

    // returns a `Ray` that has been transformed by the matrix.
    Ray transform_ray(const Ray&, const Matrix<float, 4, 4>&);

    // returns a `Ray` that has been transformed by the inverse of the supplied `Transform`
    Ray inverse_transform_ray(const Ray&, const Transform&);
}

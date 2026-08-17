#include "ray.h"

#include <liboscar/maths/constants.h>
#include <liboscar/maths/geometric_functions.h>

bool osc::is_colinear_and_codirectional(const Ray& a, const Ray& b, float tolerance)
{
    if (not is_codirectional(a.direction, b.direction, tolerance)) {
        return false;
    }
    const Vector3 origin_delta = b.origin - a.origin;
    const float len2 = length2(origin_delta);
    if (len2 <= epsilon_v<float>) {
        return true;
    }
    return is_parallel(origin_delta / sqrt(len2), a.direction, tolerance);
}

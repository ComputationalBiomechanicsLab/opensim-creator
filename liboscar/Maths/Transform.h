#pragma once

#include <liboscar/Maths/Quat.h>
#include <liboscar/Maths/Vec3.h>

#include <ostream>

namespace osc
{
    // packaged-up SQT transform (scale -> rotate -> translate)
    struct Transform final {

        constexpr Transform with_translation(const Vec3& new_translation) const
        {
            return Transform{.scale = scale, .rotation = rotation, .translation = new_translation};
        }

        constexpr Transform with_rotation(const Quat& new_rotation) const
        {
            return Transform{.scale = scale, .rotation = new_rotation, .translation = translation};
        }

        constexpr Transform with_scale(const Vec3& new_scale) const
        {
            return Transform{.scale = new_scale, .rotation = rotation, .translation = translation};
        }

        constexpr Transform with_scale(float new_scale) const
        {
            return Transform{.scale = Vec3{new_scale}, .rotation = rotation, .translation = translation};
        }

        friend bool operator==(const Transform&, const Transform&) = default;

        Vec3 scale{1.0f};
        Quat rotation{};
        Vec3 translation{};
    };

    // applies the transform to a point vector (equivalent to `transform_point`)
    constexpr Vec3 operator*(const Transform& transform, Vec3 point)
    {
        point *= transform.scale;
        point  = transform.rotation * point;
        point += transform.translation;
        return point;
    }

    template<typename T>
    constexpr T identity();

    template<>
    constexpr Transform identity<Transform>()
    {
        return Transform{};
    }

    inline std::ostream& operator<<(std::ostream& out, const Transform& transform)
    {
        return out << "Transform(translation = " << transform.translation << ", rotation = " << transform.rotation << ", scale = " << transform.scale << ')';
    }
}

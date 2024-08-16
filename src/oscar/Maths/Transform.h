#pragma once

#include <oscar/Maths/Quat.h>
#include <oscar/Maths/Vec3.h>

#include <ostream>
#include <sstream>
#include <string>
#include <utility>

namespace osc
{
    // packaged-up SQT transform (scale -> rotate -> translate)
    struct Transform final {

        constexpr Transform with_position(const Vec3& position_) const
        {
            return Transform{.scale = scale, .rotation = rotation, .position = position_};
        }

        constexpr Transform with_rotation(const Quat& rotation_) const
        {
            return Transform{.scale = scale, .rotation = rotation_, .position = position};
        }

        constexpr Transform with_scale(const Vec3& scale_) const
        {
            return Transform{.scale = scale_, .rotation = rotation, .position = position};
        }

        constexpr Transform with_scale(float scale_) const
        {
            return Transform{.scale = Vec3{scale_}, .rotation = rotation, .position = position};
        }

        friend bool operator==(const Transform&, const Transform&) = default;

        Vec3 scale{1.0f};
        Quat rotation{};
        Vec3 position{};
    };

    // applies the transform to a point vector (equivalent to `transform_point`)
    constexpr Vec3 operator*(const Transform& transform, Vec3 point)
    {
        point *= transform.scale;
        point  = transform.rotation * point;
        point += transform.position;
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
        return out << "Transform(position = " << transform.position << ", rotation = " << transform.rotation << ", scale = " << transform.scale << ')';
    }

    inline std::string to_string(const Transform& transform)
    {
        std::stringstream ss;
        ss << transform;
        return std::move(ss).str();
    }
}

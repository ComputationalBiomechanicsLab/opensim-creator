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

    // applies the transform to a point vector (equivalent to `TransformPoint`)
    constexpr Vec3 operator*(const Transform& t, Vec3 point)
    {
        point *= t.scale;
        point  = t.rotation * point;
        point += t.position;
        return point;
    }

    template<typename T>
    constexpr T identity();

    template<>
    constexpr Transform identity<Transform>()
    {
        return Transform{};
    }

    inline std::ostream& operator<<(std::ostream& o, const Transform& t)
    {
        return o << "Transform(position = " << t.position << ", rotation = " << t.rotation << ", scale = " << t.scale << ')';
    }

    inline std::string to_string(const Transform& t)
    {
        std::stringstream ss;
        ss << t;
        return std::move(ss).str();
    }
}

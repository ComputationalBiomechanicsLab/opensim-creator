#pragma once

#include <oscar/Maths/Quat.h>
#include <oscar/Maths/Vec3.h>

#include <ostream>
#include <sstream>
#include <string>
#include <utility>

namespace osc
{
    // packaged-up SQT transform (orthogonal scale -> rotate -> translate)
    struct Transform final {

        constexpr Transform with_position(Vec3 const& position_) const
        {
            return Transform{.scale = scale, .rotation = rotation, .position = position_};
        }

        constexpr Transform with_rotation(Quat const& rotation_) const
        {
            return Transform{.scale = scale, .rotation = rotation_, .position = position};
        }

        constexpr Transform with_scale(Vec3 const& scale_) const
        {
            return Transform{.scale = scale_, .rotation = rotation, .position = position};
        }

        constexpr Transform with_scale(float scale_) const
        {
            return Transform{.scale = Vec3{scale_}, .rotation = rotation, .position = position};
        }

        friend bool operator==(Transform const&, Transform const&) = default;

        Vec3 scale{1.0f};
        Quat rotation{};
        Vec3 position{};
    };

    // applies the transform to a point vector (equivalent to `TransformPoint`)
    constexpr Vec3 operator*(Transform const& t, Vec3 p)
    {
        p *= t.scale;
        p = t.rotation * p;
        p += t.position;
        return p;
    }

    template<typename T>
    constexpr T identity();

    template<>
    constexpr Transform identity<Transform>()
    {
        return Transform{};
    }

    inline std::ostream& operator<<(std::ostream& o, Transform const& t)
    {
        return o << "Transform(position = " << t.position << ", rotation = " << t.rotation << ", scale = " << t.scale << ')';
    }

    inline std::string to_string(Transform const& t)
    {
        std::stringstream ss;
        ss << t;
        return std::move(ss).str();
    }
}

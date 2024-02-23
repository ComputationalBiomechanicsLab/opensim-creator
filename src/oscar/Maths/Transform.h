#pragma once

#include <oscar/Maths/Quat.h>
#include <oscar/Maths/Vec3.h>

#include <iosfwd>

namespace osc
{
    // packaged-up "transform" (orthogonal scale -> rotate -> translate)
    struct Transform final {

        // returns a new transform which is the same as the existing one, but with the
        // provided position
        constexpr Transform withPosition(Vec3 const& position_) const
        {
            return Transform{.scale = scale, .rotation = rotation, .position = position_};
        }

        // returns a new transform which is the same as the existing one, but with
        // the provided rotation
        constexpr Transform withRotation(Quat const& rotation_) const
        {
            return Transform{.scale = scale, .rotation = rotation_, .position = position};
        }

        // returns a new transform which is the same as the existing one, but with
        // the provided scale
        constexpr Transform withScale(Vec3 const& scale_) const
        {
            return Transform{.scale = scale_, .rotation = rotation, .position = position};
        }

        // returns a new transform which is the same as the existing one, but with
        // the provided scale (same for all axes)
        constexpr Transform withScale(float scale_) const
        {
            return Transform{.scale = Vec3{scale_}, .rotation = rotation, .position = position};
        }

        friend bool operator==(Transform const&, Transform const&) = default;

        Vec3 scale{1.0f};
        Quat rotation{};
        Vec3 position{};
    };

    template<typename T>
    constexpr T Identity();

    template<>
    constexpr Transform Identity()
    {
        return Transform{};
    }

    // pretty-prints a `Transform` for readability
    std::ostream& operator<<(std::ostream&, Transform const&);

    // applies the transform to a point vector (equivalent to `TransformPoint`)
    Vec3 operator*(Transform const&, Vec3 const&);

    // performs component-wise addition of two transforms
    Transform& operator+=(Transform&, Transform const&);

    // performs component-wise scalar division
    Transform& operator/=(Transform&, float);
}

#pragma once

#include <oscar/Maths/Quat.hpp>
#include <oscar/Maths/Vec3.hpp>

#include <iosfwd>

namespace osc
{
    // packaged-up "transform" (orthogonal scale -> rotate -> translate)
    struct Transform final {

        // default-construct as an identity transform
        constexpr Transform() = default;

        // construct at a given position with an identity rotation and scale
        constexpr explicit Transform(Vec3 const& position_) noexcept :
            position{position_}
        {
        }

        // construct at a given position and rotation with an identity scale
        constexpr Transform(Vec3 const& position_,
                            Quat const& rotation_) noexcept :
            rotation{rotation_},
            position{position_}
        {
        }

        // construct at a given position, rotation, and scale
        constexpr Transform(Vec3 const& position_,
                            Quat const& rotation_,
                            Vec3 const& scale_) noexcept :
            scale{scale_},
            rotation{rotation_},
            position{position_}
        {
        }

        // returns a new transform which is the same as the existing one, but with the
        // provided position
        constexpr Transform withPosition(Vec3 const& position_) const noexcept
        {
            return Transform{position_, rotation, scale};
        }

        // returns a new transform which is the same as the existing one, but with
        // the provided rotation
        constexpr Transform withRotation(Quat const& rotation_) const noexcept
        {
            return Transform{position, rotation_, scale};
        }

        // returns a new transform which is the same as the existing one, but with
        // the provided scale
        constexpr Transform withScale(Vec3 const& scale_) const noexcept
        {
            return Transform{position, rotation, scale_};
        }

        // returns a new transform which is the same as the existing one, but with
        // the provided scale (same for all axes)
        constexpr Transform withScale(float scale_) const noexcept
        {
            return Transform{position, rotation, {scale_, scale_, scale_}};
        }

        friend bool operator==(Transform const&, Transform const&) = default;

        Vec3 scale = {1.0f, 1.0f, 1.0f};
        Quat rotation = {1.0f, 0.0f, 0.0f, 0.0f};
        Vec3 position = {0.0f, 0.0f, 0.0f};
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
    Vec3 operator*(Transform const&, Vec3 const&) noexcept;

    // performs component-wise addition of two transforms
    Transform& operator+=(Transform&, Transform const&) noexcept;

    // performs component-wise scalar division
    Transform& operator/=(Transform&, float) noexcept;
}

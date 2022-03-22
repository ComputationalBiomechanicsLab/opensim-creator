#pragma once

#include <glm/vec3.hpp>
#include <glm/gtx/quaternion.hpp>

#include <iosfwd>

namespace osc
{
    // packaged-up "transform" (orthogonal scale -> rotate -> translate)
    struct Transform final {
        glm::vec3 position{0.0f, 0.0f, 0.0f};
        glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
        glm::vec3 scale{1.0f, 1.0f, 1.0f};
    
        // default-construct as an identity transform
        constexpr Transform() noexcept = default;
    
        // construct at a given position with an identity rotation and scale
        constexpr explicit Transform(glm::vec3 const& position_) noexcept :
            position{position_}
        {
        }
    
        // construct at a given position and rotation with an identity scale
        constexpr Transform(glm::vec3 const& position_,
                            glm::quat const& rotation_) noexcept :
            position{position_},
            rotation{rotation_}
        {
        }
    
        // construct at a given position, rotation, and scale
        constexpr Transform(glm::vec3 const& position_,
                            glm::quat const& rotation_,
                            glm::vec3 const& scale_) noexcept :
            position{position_},
            rotation{rotation_},
            scale{scale_}
        {
        }
    
        // returns a new transform which is the same as the existing one, but with the
        // provided position
        constexpr Transform withPosition(glm::vec3 const& position_) const noexcept
        {
            return Transform{position_, rotation, scale};
        }
    
        // returns a new transform which is the same as the existing one, but with
        // the provided rotation
        constexpr Transform withRotation(glm::quat const& rotation_) const noexcept
        {
            return Transform{position, rotation_, scale};
        }
    
        // returns a new transform which is the same as the existing one, but with
        // the provided scale
        constexpr Transform withScale(glm::vec3 const& scale_) const noexcept
        {
            return Transform{position, rotation, scale_};
        }
    
        // returns a new transform which is the same as the existing one, but with
        // the provided scale (same for all axes)
        constexpr Transform withScale(float scale_) const noexcept
        {
            return Transform{position, rotation, {scale_, scale_, scale_}};
        }
    };
    
    // pretty-prints a `Transform` for readability
    std::ostream& operator<<(std::ostream& o, Transform const&);

    // applies the transform to a point vector (equivalent to `transformPoint`)
    glm::vec3 operator*(Transform const&, glm::vec3 const&);
    
    // performs component-wise addition of two transforms
    Transform& operator+=(Transform&, Transform const&) noexcept;
    
    // performs component-wise scalar division
    Transform& operator/=(Transform&, float) noexcept;
}

#pragma once

#include <oscar/Graphics/Unorm.h>
#include <oscar/Maths/Scalar.h>

#include <cstddef>
#include <cstdint>

namespace osc
{
    // A normalized unsigned 8-bit integer that can be used to store a floating-point
    // number in the (clamped) range [0.0f, 1.0f]
    //
    // see: https://www.khronos.org/opengl/wiki/Normalized_Integer
    class Unorm8 : public Unorm<uint8_t> {
    public:
        using Unorm<uint8_t>::Unorm;

        constexpr Unorm8(std::byte raw_value) :
            Unorm8{static_cast<uint8_t>(raw_value)}
        {}

        constexpr std::byte byte() const
        {
            return static_cast<std::byte>(raw_value());
        }
    };

    // tag `Unorm8` as scalar-like, so that other parts of the codebase (e.g.
    // vectors, matrices) accept it
    template<>
    struct IsScalar<Unorm8> final {
        static constexpr bool value = true;
    };
}

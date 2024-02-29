#pragma once

#include <oscar/Maths/Qua.h>

#include <cstdint>

namespace osc
{
    using Quat = Qua<float>;
    using Quatf = Qua<float>;
    using Quatd = Qua<double>;
    using Quati = Qua<int>;
    using Quatu32 = Qua<uint32_t>;

    template<typename T>
    constexpr T identity();

    template<>
    constexpr Quat identity<Quat>()
    {
        return Quat{};
    }
}

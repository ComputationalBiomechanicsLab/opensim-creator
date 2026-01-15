#pragma once

#include <liboscar/maths/qua.h>

#include <cstdint>

namespace osc
{
    using Quaternion = Qua<float>;
    using Quaternionf = Qua<float>;
    using Quaterniond = Qua<double>;
    using Quaternioni = Qua<int>;
    using Quaternionu32 = Qua<uint32_t>;

    template<typename T>
    constexpr T identity();

    template<>
    constexpr Quaternion identity<Quaternion>()
    {
        return Quaternion{};
    }
}

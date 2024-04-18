#pragma once

#include <oscar/Maths/Angle.h>
#include <oscar/Maths/Functors.h>
#include <oscar/Maths/Vec.h>

#include <cmath>
#include <concepts>

namespace osc
{
    template<std::floating_point GenType>
    GenType sin(GenType v)
    {
        return std::sin(v);
    }

    template<std::floating_point Rep, AngularUnitTraits Units>
    Rep sin(Angle<Rep, Units> v)
    {
        return sin(RadiansT<Rep>{v}.count());
    }

    template<size_t L, std::floating_point T>
    Vec<L, T> sin(const Vec<L, T>& v)
    {
        return map(v, sin<T>);
    }

    template<std::floating_point GenType>
    GenType cos(GenType v)
    {
        return std::cos(v);
    }

    template<std::floating_point Rep, AngularUnitTraits Units>
    Rep cos(Angle<Rep, Units> v)
    {
        return cos(RadiansT<Rep>{v}.count());
    }

    template<size_t L, std::floating_point T>
    Vec<L, T> cos(const Vec<L, T>& v)
    {
        return map(v, cos<T>);
    }

    template<std::floating_point GenType>
    GenType tan(GenType v)
    {
        return std::tan(v);
    }

    template<std::floating_point Rep, AngularUnitTraits Units>
    Rep tan(Angle<Rep, Units> v)
    {
        return tan(RadiansT<Rep>{v}.count());
    }

    template<size_t L, std::floating_point T>
    Vec<L, T> tan(const Vec<L, T>& v)
    {
        return elementwise_map(v, tan);
    }

    template<std::floating_point Rep>
    RadiansT<Rep> atan(Rep v)
    {
        return RadiansT<Rep>{std::atan(v)};
    }

    template<std::floating_point Rep>
    RadiansT<Rep> acos(Rep num)
    {
        return RadiansT<Rep>{std::acos(num)};
    }

    template<std::floating_point Rep>
    RadiansT<Rep> asin(Rep num)
    {
        return RadiansT<Rep>{std::asin(num)};
    }

    template<std::floating_point Rep>
    RadiansT<Rep> atan2(Rep x, Rep y)
    {
        return RadiansT<Rep>{std::atan2(x, y)};
    }
}

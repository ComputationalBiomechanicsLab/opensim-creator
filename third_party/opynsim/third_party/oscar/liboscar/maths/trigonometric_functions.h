#pragma once

#include <liboscar/maths/angle.h>
#include <liboscar/maths/functors.h>
#include <liboscar/maths/vector.h>

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
    Rep sin(Angle<Rep, Units> v) { return sin(RadiansT<Rep>{v}.count()); }

    template<std::floating_point T, size_t N>
    Vector<T, N> sin(const Vector<T, N>& v)
    {
        return map(v, sin<T>);
    }

    template<std::floating_point Rep, AngularUnitTraits Units, size_t N>
    Vector<Rep, N> sin(const Vector<Angle<Rep, Units>, N>& v)
    {
        return map(v, sin<Rep, Units>);
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

    template<std::floating_point T, size_t N>
    Vector<T, N> cos(const Vector<T, N>& v)
    {
        return map(v, cos<T>);
    }

    template<std::floating_point Rep, AngularUnitTraits Units, size_t N>
    Vector<Rep, N> cos(const Vector<Angle<Rep, Units>, N>& v)
    {
        return map(v, cos<Rep, Units>);
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

    template<std::floating_point T, size_t N>
    Vector<T, N> tan(const Vector<T, N>& v)
    {
        return map(v, tan<T>);
    }

    template<std::floating_point Rep, AngularUnitTraits Units, size_t N>
    Vector<Rep, N> tan(const Vector<Angle<Rep, Units>, N>& v)
    {
        return map(v, tan<Rep, Units>);
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

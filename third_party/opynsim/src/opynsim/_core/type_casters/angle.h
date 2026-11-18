#pragma once

#include <liboscar/maths/angle.h>
#include <nanobind/nanobind.h>

template<>
struct nanobind::detail::type_caster<osc::Radians> final {

    using cpp_type = osc::Radians;
    using python_input_type = double;
    using python_output_type = double;

    NB_TYPE_CASTER(cpp_type, const_name("Radians"))

    bool from_python(handle src, uint8_t flags, cleanup_list* cleanup_list) noexcept
    {
        type_caster<double> caster{};
        if (not caster.from_python(src, flags, cleanup_list)) {
            return false;
        }
        value = cpp_type(static_cast<double>(caster.value));
        return true;
    }

    static handle from_cpp(const cpp_type& src, rv_policy policy, cleanup_list* cleanup) noexcept
    {
        return nanobind::detail::type_caster<double>::from_cpp(
            static_cast<double>(src.count()), policy, cleanup);
    }
};

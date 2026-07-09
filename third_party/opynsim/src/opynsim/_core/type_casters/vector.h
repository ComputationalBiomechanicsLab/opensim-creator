#pragma once

#include <src/opynsim/_core/nb_helpers.h>

#include <liboscar/maths/vector.h>
#include <nanobind/nanobind.h>
#pragma warning(push)
#pragma warning(disable : 4702) // Disable "unreachable code"
#include <nanobind/ndarray.h>
#pragma warning(pop)

#include <array>
#include <tuple>

template<>
struct nanobind::detail::type_caster<osc::Vector3f> final {

    using cpp_type = osc::Vector3f;
    using value_type = cpp_type::value_type;
    using python_input_type = ndarray<float, shape<3>, c_contig>;
    using python_output_type = ndarray<float, numpy, shape<3>>;

    NB_TYPE_CASTER(cpp_type, const_name("typename numpy.ndarray[dtype=float32, shape=(3,)]"));

    bool from_python(handle src, uint8_t flags, cleanup_list* cleanup_list) noexcept
    {
        type_caster<python_input_type> caster;
        if (not caster.from_python(src, flags, cleanup_list)) {
            return false;
        }

        static_assert(std::tuple_size_v<cpp_type> == 3);
        const value_type* data = static_cast<const python_input_type&>(caster).data();
        value = cpp_type{data[0], data[1], data[2]};
        return true;
    }

    static handle from_cpp(const cpp_type& src, rv_policy, cleanup_list*) noexcept
    {
        // Copy vector into a capsule and return it as a suitable ndarray

        static_assert(std::tuple_size_v<cpp_type> == 3);
        auto data = std::unique_ptr<value_type[]>(new value_type[3]{src[0], src[1], src[2]});
        value_type* ptr = data.get();
        const auto shape = std::to_array({ 3uz });

        python_output_type ndarray{ptr, shape.size(), shape.data(), opyn::to_capsule(std::move(data))};

        return ndarray.cast().release();
    }
};

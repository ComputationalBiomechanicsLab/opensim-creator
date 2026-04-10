#pragma once

#include <concepts>

namespace osc
{
    // Is satisfied when `T` can be used as a color component. In oscar,
    // color components have to be fluid to interconvert between a normalized
    // floating-point and unsigned integer representation. The reason why
    // is application code can use either of the two. E.g:
    //
    //     Color32{}.r = 0xff;         // the 0xff byte means white
    //     Color{}.r   = 1.0f;         // the normalized float means white
    //     Color{}.r   = 1.2f;         // the normalized float means "more than white" (i.e. HDR)
    //     Color var = Color32{0xff};  // should be possible
    template<typename T>
    concept ColorComponent =
        std::floating_point<T> or
        (
            std::constructible_from<T, float> and
            std::assignable_from<T&, float> and
            std::constructible_from<float, const T&>
        );
}

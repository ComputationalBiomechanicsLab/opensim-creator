#pragma once

namespace osmv {
    template<typename T>
    struct Dimensions final {
        T w;
        T h;
    };

    template<typename T, typename U>
    [[nodiscard]] constexpr U aspect_ratio(Dimensions<T> d) noexcept {
        return static_cast<U>(d.w) / static_cast<U>(d.h);
    }
}

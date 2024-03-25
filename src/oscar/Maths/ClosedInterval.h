#pragma once

#include <concepts>

namespace osc
{
    // vocabulary type to describe "two fixed endpoints with no 'gaps', including the endpoints themselves"
    template<typename T>
    requires std::equality_comparable<T> and std::totally_ordered<T>
    struct ClosedInterval final {
        T lower;
        T upper;
    };
}

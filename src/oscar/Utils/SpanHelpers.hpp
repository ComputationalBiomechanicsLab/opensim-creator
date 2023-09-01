#pragma once

#include <nonstd/span.hpp>

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace osc
{
    template<typename T>
    nonstd::span<std::byte const> ViewAsByteSpan(T const& v)
    {
        static_assert(std::is_trivially_copyable_v<T>);
        static_assert(std::is_trivially_destructible_v<T>);
        return {reinterpret_cast<std::byte const*>(&v), sizeof(T)};
    }

    template<typename T>
    nonstd::span<uint8_t const> ViewAsUint8Span(T const& v)
    {
        static_assert(std::is_trivially_copyable_v<T>);
        static_assert(std::is_trivially_destructible_v<T>);
        return {reinterpret_cast<uint8_t const*>(&v), sizeof(T)};
    }
}
#pragma once

#include <nonstd/span.hpp>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
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

    template<typename T>
    nonstd::span<uint8_t const> ViewSpanAsUint8Span(nonstd::span<T const> const& vs)
    {
        static_assert(std::is_trivially_copyable_v<T>);
        static_assert(std::is_trivially_destructible_v<T>);
        return {reinterpret_cast<uint8_t const*>(vs.data()), sizeof(T) * vs.size()};
    }

    template<typename T>
    T& At(nonstd::span<T> vs, size_t i)
    {
        if (i <= vs.size())
        {
            return vs[i];
        }
        else
        {
            throw std::out_of_range{"invalid span subscript"};
        }
    }
}

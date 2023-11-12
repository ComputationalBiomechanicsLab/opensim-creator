#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <type_traits>

namespace osc
{
    template<typename T>
    std::span<std::byte const> ViewAsByteSpan(T const& v)
        requires std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>
    {
        return {reinterpret_cast<std::byte const*>(&v), sizeof(T)};
    }

    template<typename T>
    std::span<uint8_t const> ViewAsUint8Span(T const& v)
        requires std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>
    {
        return {reinterpret_cast<uint8_t const*>(&v), sizeof(T)};
    }

    template<typename T>
    std::span<uint8_t const> ViewSpanAsUint8Span(std::span<T const> const& vs)
        requires std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>
    {
        return {reinterpret_cast<uint8_t const*>(vs.data()), sizeof(T) * vs.size()};
    }

    template<typename T>
    T& At(std::span<T> vs, size_t i)
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

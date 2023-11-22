#pragma once

#include <oscar/Utils/Concepts.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>

namespace osc
{
    template<BitCastable T>
    std::span<std::byte const> ObjectRepresentationToByteSpan(T const& v)
    {
        return {reinterpret_cast<std::byte const*>(&v), sizeof(T)};
    }

    template<BitCastable T>
    std::span<uint8_t const> ObjectRepresentationToUint8Span(T const& v)
    {
        return {reinterpret_cast<uint8_t const*>(&v), sizeof(T)};
    }

    template<BitCastable T>
    std::span<uint8_t const> DataToUint8Span(std::span<T const> vs)
    {
        return {reinterpret_cast<uint8_t const*>(vs.data()), sizeof(T) * vs.size()};
    }

    template<ContiguousContainer Container>
    std::span<uint8_t const> DataToUint8Span(Container const& c)
        requires BitCastable<typename Container::value_type>
    {
        return {reinterpret_cast<uint8_t const*>(c.data()), sizeof(Container::value_type) * c.size()};
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

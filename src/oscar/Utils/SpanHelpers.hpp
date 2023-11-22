#pragma once

#include <oscar/Utils/Concepts.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>

namespace osc
{
    template<ObjectRepresentationByte Byte = std::byte, BitCastable T>
    std::span<Byte const> ViewObjectRepresentation(T const& v)
    {
        return {reinterpret_cast<Byte const*>(&v), sizeof(T)};
    }

    template<ObjectRepresentationByte Byte = std::byte, ContiguousContainer Container>
    std::span<Byte const> ViewObjectRepresentations(Container const& c)
        requires BitCastable<typename Container::value_type>
    {
        return {reinterpret_cast<Byte const*>(c.data()), sizeof(typename Container::value_type) * c.size()};
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

#pragma once

#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/Concepts.h>
#include <oscar/Utils/StdVariantHelpers.h>

#include <cstdint>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <span>
#include <stdexcept>
#include <variant>

namespace osc
{
    // a span-like view over mesh indices
    //
    // for perf reasons, runtime mesh indices can be stored in either a 16-bit or 32-bit format
    // the mesh class exposes this fact by returning this view class, which must be checked at
    // runtime by calling code

    class MeshIndicesView final {
    private:
        using U32PtrOrU16Ptr = std::variant<uint16_t const*, uint32_t const*>;
    public:
        class Iterator final {
        public:
            using difference_type = ptrdiff_t;
            using value_type = uint32_t;
            using pointer = void;
            using reference = value_type;
            using iterator_category = std::forward_iterator_tag;

            Iterator() = default;

            explicit Iterator(U32PtrOrU16Ptr ptr) : m_Ptr{ptr}
            {}

            value_type operator*() const
            {
                return std::visit(Overload{
                    [](auto const* ptr) { return static_cast<value_type>(*ptr); }
                }, m_Ptr);
            }

            friend bool operator==(Iterator const&, Iterator const&) = default;

            Iterator& operator++()
            {
                return *this += 1;
            }

            Iterator operator++(int)
            {
                Iterator copy{*this};
                ++(*this);
                return copy;
            }

            Iterator& operator+=(difference_type v)
            {
                std::visit(Overload{
                    [v](auto& ptr) { ptr += v; }
                }, m_Ptr);
                return *this;
            }

            value_type operator[](difference_type v) const
            {
                return std::visit(Overload{
                    [v](auto const* ptr) { return static_cast<value_type>(ptr[v]); }
                }, m_Ptr);
            }
        private:
            U32PtrOrU16Ptr m_Ptr{static_cast<uint16_t const*>(nullptr)};
        };

        using value_type = uint32_t;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        using reference = value_type;
        using const_reference = value_type;
        using pointer = void;
        using const_pointer = void;
        using iterator = Iterator;
        using const_iterator = Iterator;

        MeshIndicesView() = default;

        MeshIndicesView(uint16_t const* ptr, size_t size) :
            m_Ptr{ptr},
            m_Size{size}
        {}

        MeshIndicesView(uint32_t const* ptr, size_t size) :
            m_Ptr{ptr},
            m_Size{size}
        {}

        template<std::ranges::contiguous_range Range>
        MeshIndicesView(Range const& range)
            requires IsAnyOf<typename Range::value_type, uint16_t, uint32_t>

            : MeshIndicesView{std::ranges::data(range), std::ranges::size(range)}
        {
        }

        bool isU16() const
        {
            return std::holds_alternative<uint16_t const*>(m_Ptr);
        }

        bool isU32() const
        {
            return std::holds_alternative<uint32_t const*>(m_Ptr);
        }

        [[nodiscard]] bool empty() const
        {
            return size() == 0;
        }

        size_type size() const
        {
            return m_Size;
        }

        std::span<uint16_t const> toU16Span() const
        {
            return {std::get<uint16_t const*>(m_Ptr), m_Size};
        }

        std::span<uint32_t const> toU32Span() const
        {
            return {std::get<uint32_t const*>(m_Ptr), m_Size};
        }

        value_type operator[](size_type i) const
        {
            return begin()[i];
        }

        value_type at(size_type i) const
        {
            if (i >= size()) {
                throw std::out_of_range{"attempted to access a MeshIndicesView with an invalid index"};
            }
            return this->operator[](i);
        }

        Iterator begin() const
        {
            return Iterator{m_Ptr};
        }

        Iterator end() const
        {
            return std::visit(Overload{
                [size = m_Size](auto const* ptr) { return Iterator{ptr + size}; },
            }, m_Ptr);
        }

    private:
        U32PtrOrU16Ptr m_Ptr{static_cast<uint16_t const*>(nullptr)};
        size_t m_Size = 0;
    };
}

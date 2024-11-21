#pragma once

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
        using U32PtrOrU16Ptr = std::variant<const uint16_t*, const uint32_t*>;
    public:
        class Iterator final {
        public:
            using difference_type = ptrdiff_t;
            using value_type = uint32_t;
            using pointer = void;
            using reference = value_type;
            using iterator_category = std::forward_iterator_tag;

            Iterator() = default;

            explicit Iterator(U32PtrOrU16Ptr ptr) : ptr_{ptr}
            {}

            value_type operator*() const
            {
                return std::visit(Overload{
                    [](const auto* ptr) { return static_cast<value_type>(*ptr); }
                }, ptr_);
            }

            friend bool operator==(const Iterator&, const Iterator&) = default;

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

            Iterator& operator+=(difference_type n)
            {
                std::visit(Overload{
                    [n](auto& ptr) { ptr += n; }
                }, ptr_);
                return *this;
            }

            value_type operator[](difference_type pos) const
            {
                return std::visit(Overload{
                    [pos](const auto* ptr) { return static_cast<value_type>(ptr[pos]); }
                }, ptr_);
            }
        private:
            U32PtrOrU16Ptr ptr_{static_cast<const uint16_t*>(nullptr)};
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

        MeshIndicesView(const uint16_t* ptr, size_t size) :
            ptr_{ptr},
            size_{size}
        {}

        MeshIndicesView(const uint32_t* ptr, size_t size) :
            ptr_{ptr},
            size_{size}
        {}

        template<std::ranges::contiguous_range Range>
        requires IsAnyOf<typename Range::value_type, uint16_t, uint32_t>
        MeshIndicesView(const Range& range) :
            MeshIndicesView{std::ranges::data(range), std::ranges::size(range)}
        {}

        bool is_uint16() const
        {
            return std::holds_alternative<const uint16_t*>(ptr_);
        }

        bool is_uint32() const
        {
            return std::holds_alternative<const uint32_t*>(ptr_);
        }

        [[nodiscard]] bool empty() const
        {
            return size() == 0;
        }

        size_type size() const
        {
            return size_;
        }

        std::span<const uint16_t> to_uint16_span() const
        {
            return {std::get<const uint16_t*>(ptr_), size_};
        }

        std::span<const uint32_t> to_uint32_span() const
        {
            return {std::get<const uint32_t*>(ptr_), size_};
        }

        value_type operator[](size_type pos) const
        {
            return begin()[pos];
        }

        value_type at(size_type pos) const
        {
            if (pos >= size()) {
                throw std::out_of_range{"attempted to access a MeshIndicesView with an invalid index"};
            }
            return this->operator[](pos);
        }

        Iterator begin() const
        {
            return Iterator{ptr_};
        }

        Iterator end() const
        {
            return std::visit(Overload{
                [size = size_](const auto* ptr) { return Iterator{ptr + size}; },
            }, ptr_);
        }

    private:
        U32PtrOrU16Ptr ptr_{static_cast<const uint16_t*>(nullptr)};
        size_t size_ = 0;
    };
}

#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <memory_resource>
#include <utility>
#include <vector>

namespace osc
{
    // Provides a container that behaves as an on-stack/on-heap hybrid array that:
    //
    // - Allocates up to `N` elements on the stack without using a memory allocator.
    // - Once the number of elements exeeds `N`, allocates everything (incl. the first `N`,
    //   which are moved) via an upstream memory resource (defaults to new/delete).
    //
    // This is handy when the caller believes that there's likely to be a (low) upper
    // bound on the number of elements in the container, but they cannot be 100 %
    // certain that the number of elements will never exeed that bound. E.g. it might
    // be fair to assume that a tree loaded from a data file never exceeds 16 levels
    // of depth for reasonable inputs, but there might exist unreasonable ones, so
    // having a safe fallback to an upstream allocator is useful.
    template<typename T, size_t N>
    class VariableLengthArray final {
    private:
        using underlying_vector = std::pmr::vector<T>;
    public:
        using value_type = underlying_vector::value_type;
        using allocator_type = underlying_vector::allocator_type;
        using size_type = underlying_vector::size_type;
        using difference_type = underlying_vector::difference_type;
        using reference = underlying_vector::reference;
        using const_reference = underlying_vector::const_reference;
        using pointer = underlying_vector::pointer;
        using const_pointer = underlying_vector::const_pointer;
        using iterator = underlying_vector::iterator;
        using const_iterator = underlying_vector::const_iterator;
        using reverse_iterator = underlying_vector::reverse_iterator;
        using const_reverse_iterator = underlying_vector::const_reverse_iterator;

        VariableLengthArray() :
            VariableLengthArray(std::pmr::new_delete_resource())
        {}

        explicit VariableLengthArray(std::pmr::memory_resource* upstream_allocator) :
            pool{stack_data_.data(), stack_data_.size(), upstream_allocator}
        {
            // ensures `std::pmr::vector`'s geometric growth on-push doesn't overspill
            // the pool before reaching `N`
            vector_.reserve(N);
        }

        VariableLengthArray(const VariableLengthArray& other)
            requires std::copy_constructible<T> :
            pool{stack_data_.data(), stack_data_.size(), other.pool.upstream_resource()}
        {
            vector_.reserve(N);
            vector_.assign(other.vector_.begin(), other.vector_.end());
        }

        VariableLengthArray(VariableLengthArray&& other) noexcept
            requires std::move_constructible<T> :
            pool{stack_data_.data(), stack_data_.size(), other.pool.upstream_resource()}
        {
            vector_.reserve(N);
            vector_.assign(std::make_move_iterator(other.vector_.begin()), std::make_move_iterator(other.vector_.end()));
        }

        VariableLengthArray(
            std::initializer_list<T> init,
            std::pmr::memory_resource* upstream_allocator = std::pmr::new_delete_resource())
            requires std::copy_constructible<T> :
            VariableLengthArray{upstream_allocator}
        {
            vector_.assign(init.begin(), init.end());
        }

        VariableLengthArray& operator=(const VariableLengthArray& other)
            requires std::is_copy_assignable_v<T>
        {
            vector_.assign(other.begin(), other.end());
            return *this;
        }

        VariableLengthArray& operator=(VariableLengthArray&& other) noexcept
            requires std::is_move_assignable_v<T>
        {
            vector_.assign(std::make_move_iterator(other.vector_.begin()), std::make_move_iterator(other.vector_.end()));
            return *this;
        }

        reference operator[](size_type pos) { return vector_[pos]; }
        const_reference operator[](size_type pos) const { return vector_[pos]; }

        reference front() { return vector_.front(); }
        const_reference front() const { return vector_.front(); }

        iterator begin() noexcept { return vector_.begin(); }
        const_iterator begin() const noexcept { return vector_.begin(); }
        const_iterator cbegin() const noexcept { return vector_.cbegin(); }
        iterator end() noexcept { return vector_.end(); }
        const_iterator end() const noexcept { return vector_.end(); }
        const_iterator cend() const noexcept { return vector_.cend(); }
        reverse_iterator rbegin() noexcept { return vector_.rbegin(); }
        const_reverse_iterator rbegin() const noexcept { return vector_.rbegin(); }
        const_reverse_iterator crbegin() const noexcept { return vector_.crbegin(); }
        reverse_iterator rend() noexcept { return vector_.rend(); }
        const_reverse_iterator rend() const noexcept { return vector_.rend(); }
        const_reverse_iterator crend() const noexcept { return vector_.crend(); }

        [[nodiscard]] bool empty() const noexcept { return vector_.empty(); }
        size_type size() const noexcept { return vector_.size(); }

        void clear() { vector_.clear(); }

        void push_back(const T& value) { vector_.push_back(value); }
        void push_back(T&& value) { vector_.push_back(std::move(value)); }

        friend bool operator==(const VariableLengthArray& lhs, const VariableLengthArray& rhs)
        {
            return lhs.vector_ == rhs.vector_;
        }
    private:
        alignas(T) std::array<std::byte, N * sizeof(T)> stack_data_;
        std::pmr::monotonic_buffer_resource pool{stack_data_.data(), stack_data_.size(), std::pmr::new_delete_resource()};
        std::pmr::vector<T> vector_ = std::pmr::vector<T>(&pool);
    };
}

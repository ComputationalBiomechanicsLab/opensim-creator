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
    // - Once the number of elements exceeds `N`, allocates all elements (incl. existing
    //   elements, which are moved) via an upstream memory resource, which defaults to
    //   a `std::pmr::new_delete_resource`
    //
    // This is handy when the caller believes that there's likely to be a (low) upper
    // bound on the number of elements in the container, but they cannot be 100 %
    // certain that the number of elements will never exceed that bound. E.g. it might
    // be fair to assume that a tree loaded from a data file never exceeds 16 levels
    // of depth for reasonable inputs, but there might exist unreasonable ones, so
    // having a safe fallback to an upstream allocator is useful.
    template<typename T, size_t N>
    class VariableLengthArray final {
    private:
        using underlying_vector = std::pmr::vector<T>;
    public:
        using value_type = typename underlying_vector::value_type;
        using allocator_type = typename underlying_vector::allocator_type;
        using size_type = typename underlying_vector::size_type;
        using difference_type = typename underlying_vector::difference_type;
        using reference = typename underlying_vector::reference;
        using const_reference = typename underlying_vector::const_reference;
        using pointer = typename underlying_vector::pointer;
        using const_pointer = typename underlying_vector::const_pointer;
        using iterator = typename underlying_vector::iterator;
        using const_iterator = typename underlying_vector::const_iterator;
        using reverse_iterator = typename underlying_vector::reverse_iterator;
        using const_reverse_iterator = typename underlying_vector::const_reverse_iterator;

        explicit VariableLengthArray(std::pmr::memory_resource* upstream_allocator = std::pmr::new_delete_resource()) :
            pool{stack_data_.data(), stack_data_.size(), upstream_allocator}
        {
            vector_.reserve(N);  // reserve the stack as one unit of allocation
        }

        VariableLengthArray(const VariableLengthArray& other)
            requires std::copy_constructible<T> :
            pool{stack_data_.data(), stack_data_.size(), other.pool.upstream_resource()}
        {
            vector_.reserve(N);  // reserve the stack as one unit of allocation
            vector_.assign(other.vector_.begin(), other.vector_.end());
        }

        VariableLengthArray(VariableLengthArray&& other) noexcept
            requires std::move_constructible<T> :
            pool{stack_data_.data(), stack_data_.size(), other.pool.upstream_resource()}
        {
            vector_.reserve(N);  // reserve the stack as one unit of allocation
            vector_.assign(std::make_move_iterator(other.vector_.begin()), std::make_move_iterator(other.vector_.end()));
        }

        VariableLengthArray(
            std::initializer_list<T> init,
            std::pmr::memory_resource* upstream_allocator = std::pmr::new_delete_resource())
            requires std::copy_constructible<T> :
            pool{stack_data_.data(), stack_data_.size(), upstream_allocator}
        {
            vector_.reserve(N);  // reserve the stack as one unit of allocation
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

        friend bool operator==(const VariableLengthArray& lhs, const VariableLengthArray& rhs) { return lhs.vector_ == rhs.vector_; }

    private:
// Fix debug ABI issue from MSVC: when compiling in debug mode, MSVC will allocate a
// little container proxy at the start of the allocation unit (presumably, to track
// allocations) and that causes problems when downstream code expects spillover to
// occur at particular points (e.g. the test suite ensures this).
//
// see: https://developercommunity.visualstudio.com/t/debug-version-of-stl-is-not-excepion-safe-and-caus/77779
#if defined(_MSC_VER) && _ITERATOR_DEBUG_LEVEL > 0
        static inline constexpr size_t extra_bytes_ = []()
        {
            // from MSVC
            constexpr size_t proxy_size = sizeof(std::_Container_proxy);
            // ensure the extra bytes also account for the remainder of the buffer being used
            // for over-aligned datastructures
            constexpr size_t num_align_windows = (proxy_size + (alignof(T) - 1))/alignof(T);
            return alignof(T) * num_align_windows;
        }();
#else
        static inline constexpr size_t extra_bytes_ = 0;
#endif

        // The object representation of elements, stored on-stack when `size() <= N`
        alignas(T) std::array<std::byte, N * sizeof(T) + extra_bytes_> stack_data_;

        // A memory resource that uses `stack_data_` until `size() > N`, it then uses an upstream resource
        std::pmr::monotonic_buffer_resource pool{stack_data_.data(), stack_data_.size(), std::pmr::new_delete_resource()};

        // A vector that's backed by the above memory resource
        underlying_vector vector_ = underlying_vector(&pool);
    };
}

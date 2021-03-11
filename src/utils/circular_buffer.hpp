#pragma once

#include <array>
#include <stdexcept>
#include <utility>

template<typename T, size_t N>
class Circular_buffer final {
    static_assert(N > 1);

    std::array<T, N> storage;
    int _begin = 0;
    int _end = 0;

    template<bool IsConst>
    class Iterator final {
        T* data = nullptr;
        int pos = 0;

    public:
        constexpr Iterator(T* _data, int _pos) noexcept : data{_data}, pos{_pos} {
        }

        constexpr Iterator& operator++() noexcept {
            pos = (pos + 1) % N;
            return *this;
        }

        template<bool _IsConst = IsConst>
        [[nodiscard]] constexpr typename std::enable_if_t<_IsConst, T const&> operator*() const noexcept {
            return data[pos];
        }

        template<bool _IsConst = IsConst>
        [[nodiscard]] constexpr typename std::enable_if_t<not _IsConst, T&> operator*() noexcept {
            return data[pos];
        }

        [[nodiscard]] constexpr bool operator!=(Iterator const& other) const noexcept {
            return pos != other.pos or data != other.data;
        }
    };

public:
    using value_type = T;
    using size_type = std::size_t;
    using reference = T&;
    using const_reference = T const&;
    using iterator = Iterator<false>;
    using const_iterator = Iterator<true>;

    constexpr Circular_buffer() = default;

    // element access

    [[nodiscard]] constexpr reference at(size_type pos) {
        if (pos > size()) {
            throw std::out_of_range{"tried to access a circular buffer element outside of its range"};
        }
        return (*this)[pos];
    }

    [[nodiscard]] constexpr const_reference at(size_type pos) const {
        if (pos > size()) {
            throw std::out_of_range{"tried to access a circular buffer element outside of its range"};
        }
        return (*this)[pos];
    }

    [[nodiscard]] constexpr reference operator[](size_type pos) noexcept {
        return storage[(static_cast<size_type>(_begin) + pos) % N];
    }

    [[nodiscard]] constexpr reference front() noexcept {
        return storage[_begin];
    }

    [[nodiscard]] constexpr const_reference front() const noexcept {
        return storage[_begin];
    }

    // iterators

    [[nodiscard]] Iterator<true> begin() const noexcept {
        return {const_cast<T*>(storage.data()), _begin};
    }

    [[nodiscard]] Iterator<false> begin() noexcept {
        return {storage.data(), _begin};
    }

    [[nodiscard]] Iterator<true> cbegin() const noexcept {
        return {const_cast<T*>(storage.data()), _begin};
    }

    [[nodiscard]] Iterator<true> end() const noexcept {
        return {const_cast<T*>(storage.data()), _end};
    }

    [[nodiscard]] Iterator<false> end() noexcept {
        return {storage.data(), _end};
    }

    [[nodiscard]] Iterator<true> cend() const noexcept {
        return {const_cast<T*>(storage.data()), _end};
    }

    // capacity

    [[nodiscard]] constexpr bool empty() const noexcept {
        return _begin == _end;
    }

    [[nodiscard]] constexpr size_type size() const noexcept {
        return _end >= _begin ? _end - _begin : (N - _begin) + _end;
    }

    [[nodiscard]] constexpr size_type max_size() const noexcept {
        return N;
    }

    // modifiers

    constexpr void clear() noexcept {
        // TODO: the impl. currently relies on default construction of the elements being
        // cheap, because we assume all elements are in a valid, destructible, state

        for (T& el : *this) {
            el = T{};
        }

        _begin = 0;
        _end = 0;
    }

    template<typename... Args>
    constexpr T& emplace_back(Args&&... args) {
        int next = (_end + 1) % N;

        if (next == _begin) {
            _begin = (_begin + 1) % N;
        }

        T& rv = storage[_end] = T{std::forward<Args>(args)...};
        _end = next;

        return rv;
    }

    constexpr void push_back(T const& v) {
        emplace_back(v);
    }

    constexpr void push_back(T&& v) {
        emplace_back(std::move(v));
    }
};

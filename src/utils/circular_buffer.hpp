#pragma once

#include <array>
#include <cassert>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <utility>

template<typename T, size_t N>
class Circular_buffer final {
    static_assert(N > 1, "the internal representation of a circular buffer (it has one 'dead' entry) requires this");

    std::array<T, N> storage;
    int _begin = 0;  // idx of first element
    int _end = 0;  // first index after last element

    template<bool IsConst>
    class Iterator final {
        T* data = nullptr;
        int pos = 0;

    public:
        using difference_type = int;
        using value_type = T;
        using pointer = typename std::conditional<IsConst, T const*, T*>::type;
        using reference = typename std::conditional<IsConst, T const&, T&>::type;
        using iterator_category = std::random_access_iterator_tag;
        friend class Iterator<!IsConst>;

        constexpr Iterator() = default;

        constexpr Iterator(T* _data, int _pos) noexcept : data{_data}, pos{_pos} {
        }

        // implicit conversion from non-const iterator to a const one

        template<bool _IsConst = IsConst>
        [[nodiscard]] constexpr operator typename std::enable_if_t<!_IsConst, Iterator<true>>() const noexcept {
            return Iterator<true>{data, pos};
        }

        // LegacyIterator

        constexpr Iterator& operator++() noexcept {
            pos = (pos + 1) % N;
            return *this;
        }

        template<bool _IsConst = IsConst>
        [[nodiscard]] constexpr typename std::enable_if_t<_IsConst, T const&> operator*() const noexcept {
            return data[pos];
        }

        template<bool _IsConst = IsConst>
        [[nodiscard]] constexpr typename std::enable_if_t<!_IsConst, T&> operator*() const noexcept {
            return data[pos];
        }

        // EqualityComparable

        template<bool OtherConst>
        [[nodiscard]] constexpr bool operator!=(Iterator<OtherConst> const& other) const noexcept {
            return pos != other.pos;
        }

        template<bool OtherConst>
        [[nodiscard]] constexpr bool operator==(Iterator<OtherConst> const& other) const noexcept {
            return !(*this != other);
        }

        // LegacyInputIterator

        template<bool _IsConst = IsConst>
        [[nodiscard]] constexpr typename std::enable_if_t<_IsConst, T const*> operator->() const noexcept {
            return &data[pos];
        }

        template<bool _IsConst = IsConst>
        [[nodiscard]] constexpr typename std::enable_if_t<!_IsConst, T*> operator->() const noexcept {
            return &data[pos];
        }

        // LegacyForwardIterator

        constexpr Iterator operator++(int) noexcept {
            Iterator copy{*this};
            ++(*this);
            return copy;
        }

        // LegacyBidirectionalIterator

        constexpr Iterator& operator--() noexcept {
            pos = pos == 0 ? N - 1 : pos - 1;
            return *this;
        }

        constexpr Iterator operator--(int) noexcept {
            Iterator copy{*this};
            --(*this);
            return copy;
        }

        // LegacyRandomAccessIterator

        constexpr Iterator& operator+=(difference_type i) noexcept {
            pos = (pos + i) % N;
            return *this;
        }

        constexpr Iterator operator+(difference_type i) const noexcept {
            Iterator copy{*this};
            copy += i;
            return copy;
        }

        constexpr Iterator& operator-=(difference_type i) noexcept {
            difference_type im = (i % N);

            if (im > pos) {
                pos = N - (im - pos);
            } else {
                pos = pos - im;
            }

            return *this;
        }

        constexpr Iterator operator-(difference_type i) const noexcept {
            Iterator copy{*this};
            copy -= i;
            return copy;
        }

        template<bool OtherConst>
        constexpr difference_type operator-(Iterator<OtherConst> const& other) const noexcept {
            return pos - other.pos;
        }

        template<bool _IsConst = IsConst>
        [[nodiscard]] constexpr typename std::enable_if_t<_IsConst, T const&> operator[](difference_type i) const
            noexcept {
            return data[(pos + i) % N];
        }

        template<bool _IsConst = IsConst>
        [[nodiscard]] constexpr typename std::enable_if_t<!_IsConst, T&> operator[](difference_type i) const noexcept {
            return data[(pos + i) % N];
        }

        template<bool OtherConst>
        constexpr bool operator<(Iterator<OtherConst> const& other) const noexcept {
            return pos < other.pos;
        }

        template<bool OtherConst>
        constexpr bool operator>(Iterator<OtherConst> const& other) const noexcept {
            return pos > other.pos;
        }

        template<bool OtherConst>
        constexpr bool operator>=(Iterator<OtherConst> const& other) const noexcept {
            return !(*this < other);
        }

        template<bool OtherConst>
        constexpr bool operator<=(Iterator<OtherConst> const& other) const noexcept {
            return !(*this > other);
        }
    };

public:
    using value_type = T;
    using size_type = std::size_t;
    using reference = T&;
    using const_reference = T const&;
    using iterator = Iterator<false>;
    using const_iterator = Iterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

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

    [[nodiscard]] constexpr reference back() noexcept {
        return *rbegin();
    }

    [[nodiscard]] constexpr const_reference back() const noexcept {
        return *rbegin();
    }

    // iterators

    [[nodiscard]] constexpr const_iterator begin() const noexcept {
        return {const_cast<T*>(storage.data()), _begin};
    }

    [[nodiscard]] constexpr iterator begin() noexcept {
        return {storage.data(), _begin};
    }

    [[nodiscard]] constexpr const_iterator cbegin() const noexcept {
        return {const_cast<T*>(storage.data()), _begin};
    }

    [[nodiscard]] constexpr const_iterator end() const noexcept {
        return {const_cast<T*>(storage.data()), _end};
    }

    [[nodiscard]] constexpr iterator end() noexcept {
        return {storage.data(), _end};
    }

    [[nodiscard]] constexpr const_iterator cend() const noexcept {
        return {const_cast<T*>(storage.data()), _end};
    }

    [[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator{end()};
    }

    [[nodiscard]] constexpr reverse_iterator rbegin() noexcept {
        return reverse_iterator{end()};
    }

    [[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept {
        return const_reverse_iterator{end()};
    }

    [[nodiscard]] constexpr const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator{begin()};
    }

    [[nodiscard]] constexpr reverse_iterator rend() noexcept {
        return reverse_iterator{begin()};
    }

    [[nodiscard]] constexpr const_reverse_iterator crend() const noexcept {
        return const_reverse_iterator{begin()};
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

    constexpr iterator erase(iterator first, iterator last) {
        assert(last == end() && "TODO: can currently only erase elements from end of circular buffer");

        // TODO: the impl. currently relies on default construction of the elements being
        // cheap, because we assume all elements are in a valid, destructible, state
        for (auto it = first; it < last; ++it) {
            *it = T{};
        }

        _end -= std::distance(first, last);

        return end();
    }

    constexpr std::optional<T> try_pop_back() {
        if (empty()) {
            return std::nullopt;
        }

        _end = _end == 0 ? N - 1 : _end - 1;

        return std::move(storage[_end]);
    }
};

#pragma once

#include <array>
#include <cassert>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <utility>
#include <new>
#include <type_traits>

template<typename T, size_t N>
class CircularBuffer final {
    static_assert(N > 1, "the internal representation of a circular buffer (it has one 'dead' entry) requires this");

    // raw (byte) storage for elements
    //
    // - it's raw bytes so that the implementation doesn't
    //   require a sequence of default-constructed Ts to
    //   populate the storage
    //
    // - the circular/modulo range [_begin.._end) contains
    //   fully-constructed Ts
    //
    // - _end always points to a "dead", but valid, location
    //   in storage
    //
    // - the above constraints imply that the number of "live"
    //   elements in storage is N-1, because _end will modulo
    //   spin into position 0 once it is equal to N (this is
    //   in constrast to non-circular storage, where _end
    //   typically points one past the end of the storage range)
    // 
    // - this behavior makes the implementation simpler, because
    //   you don't have to handle _begin == _end edge cases and
    //   one-past-the end out-of-bounds checks
    using storage_bytes = std::aligned_storage_t<sizeof(T), alignof(T)>;
    std::array<storage_bytes, N> raw_storage;

    // index (T-based, not raw byte based) of the first element
    int _begin = 0;

    // first index (T-based, not raw byte based) *after* the last element
    int _end = 0;

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

    constexpr CircularBuffer() = default;
    CircularBuffer(CircularBuffer const& other) = delete;
    CircularBuffer(CircularBuffer&&) = delete;
    CircularBuffer& operator=(CircularBuffer const&) = delete;
    CircularBuffer& operator=(CircularBuffer&&) = delete;
    ~CircularBuffer() noexcept
    {
        for (T& el : *this)
        {
            el.~T();
        }
    }


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
        size_type idx = (static_cast<size_type>(_begin) + pos) % N;
        return *std::launder(reinterpret_cast<T*>(raw_storage.data() + idx));
    }

    [[nodiscard]] constexpr reference front() noexcept {
        return *std::launder(reinterpret_cast<T*>(raw_storage.data() + static_cast<size_type>(_begin)));
    }

    [[nodiscard]] constexpr const_reference front() const noexcept {
        *std::launder(reinterpret_cast<T*>(raw_storage.data() + static_cast<size_type>(_begin)));
    }

    [[nodiscard]] constexpr reference back() noexcept {
        return *rbegin();
    }

    [[nodiscard]] constexpr const_reference back() const noexcept {
        return *rbegin();
    }

    // iterators

    [[nodiscard]] constexpr const_iterator begin() const noexcept {
        // the iterator is designed to handle const-ness
        T const* const_ptr = std::launder(reinterpret_cast<T const*>(raw_storage.data()));
        return const_iterator{const_cast<T*>(const_ptr), _begin};
    }

    [[nodiscard]] constexpr iterator begin() noexcept {
        T* ptr = std::launder(reinterpret_cast<T*>(raw_storage.data()));
        return iterator{ptr, _begin};
    }

    [[nodiscard]] constexpr const_iterator cbegin() const noexcept {
        return begin();
    }

    [[nodiscard]] constexpr const_iterator end() const noexcept {
        // the iterator is designed to handle const-ness
        T const* const_ptr = std::launder(reinterpret_cast<T const*>(raw_storage.data()));
        return const_iterator{const_cast<T*>(const_ptr), _end};
    }

    [[nodiscard]] constexpr iterator end() noexcept {
        T* ptr = std::launder(reinterpret_cast<T*>(raw_storage.data()));
        return iterator{ptr, _end};
    }

    [[nodiscard]] constexpr const_iterator cend() const noexcept {
        return end();
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
    //
    // be careful here: the data is type-punned from a sequence of bytes
    // so that the backing storage does not have a strict requirement of
    // having to contain redundant default-constrcuted elements

    constexpr void clear() noexcept {
        for (T& el : *this) {
            el.~T();
        }

        _begin = 0;
        _end = 0;
    }

    template<typename... Args>
    constexpr T& emplace_back(Args&&... args) {
        int new_end = (_end + 1) % N;

        if (new_end == _begin) {
            // wraparound case: this is a fixed-size non-blocking circular
            // buffer
            //
            // there is a "dead" element in the buffer after the last element
            // but before the first (head). The head is about to become the
            // new "dead" element and should be destructed

            front().~T();
            _begin = (_begin + 1) % N;
        }

        // construct T in the old "dead" element location
        storage_bytes* ptr = raw_storage.data() + _end;
        T* constructed_el = new (ptr) T{std::forward<Args>(args)...};

        _end = new_end;

        return *constructed_el;
    }

    constexpr void push_back(T const& v) {
        emplace_back(v);
    }

    constexpr void push_back(T&& v) {
        emplace_back(std::move(v));
    }

    constexpr iterator erase(iterator first, iterator last) {
        assert(last == end() && "can currently only erase elements from end of circular buffer");

        for (auto it = first; it < last; ++it) {
            it->~T();
        }

        _end -= std::distance(first, last);

        return end();
    }

    constexpr std::optional<T> try_pop_back() {
        std::optional<T> rv = std::nullopt;

        if (empty()) {
            return rv;
        }

        rv.emplace(std::move(back()));
        _end = _end == 0 ? static_cast<int>(N) - 1 : _end - 1;

        return rv;
    }

    constexpr T pop_back() {
        if (empty()) {
            throw std::runtime_error{"tried to call Circular_buffer::pop_back on an empty circular buffer"};
        }

        T rv{std::move(back())};
        _end = _end == 0 ? static_cast<int>(N) - 1 : _end - 1;
        return rv;
    }
};

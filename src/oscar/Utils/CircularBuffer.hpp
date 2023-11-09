#pragma once

#include <array>
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <new>
#include <type_traits>

namespace osc
{
    template<typename T, size_t N>
    class CircularBuffer final {
        static_assert(N > 1, "the internal representation of a circular buffer (it has one 'dead' entry) requires this");

        // iterator implementation
        template<bool IsConst>
        class Iterator final {
            T* data = nullptr;
            ptrdiff_t pos = 0;

        public:
            using difference_type = ptrdiff_t;
            using value_type = T;
            using pointer = typename std::conditional<IsConst, T const*, T*>::type;
            using reference = typename std::conditional<IsConst, T const&, T&>::type;
            using iterator_category = std::random_access_iterator_tag;
            friend class Iterator<!IsConst>;

            constexpr Iterator(T* _data, ptrdiff_t _pos) noexcept :
                data{_data},
                pos{_pos}
            {
            }

            // implicit conversion from non-const iterator to a const one

            template<bool _IsConst = IsConst>
            [[nodiscard]] constexpr operator typename std::enable_if_t<!_IsConst, Iterator<true>>() const noexcept
            {
                return Iterator<true>{data, pos};
            }

            // LegacyIterator

            constexpr Iterator& operator++() noexcept
            {
                pos = (pos + 1) % N;
                return *this;
            }

            template<bool _IsConst = IsConst>
            [[nodiscard]] constexpr typename std::enable_if_t<_IsConst, T const&> operator*() const noexcept
            {
                return data[pos];
            }

            template<bool _IsConst = IsConst>
            [[nodiscard]] constexpr typename std::enable_if_t<!_IsConst, T&> operator*() const noexcept
            {
                return data[pos];
            }

            // EqualityComparable

            template<bool OtherConst>
            [[nodiscard]] constexpr friend bool operator!=(Iterator const& lhs, Iterator<OtherConst> const& rhs) noexcept
            {
                return lhs.pos != rhs.pos;
            }

            template<bool OtherConst>
            [[nodiscard]] constexpr friend bool operator==(Iterator const& lhs, Iterator<OtherConst> const& rhs) noexcept
            {
                return !(lhs != rhs);
            }

            // LegacyInputIterator

            template<bool _IsConst = IsConst>
            [[nodiscard]] constexpr typename std::enable_if_t<_IsConst, T const*> operator->() const noexcept
            {
                return &data[pos];
            }

            template<bool _IsConst = IsConst>
            [[nodiscard]] constexpr typename std::enable_if_t<!_IsConst, T*> operator->() const noexcept
            {
                return &data[pos];
            }

            // LegacyForwardIterator

            constexpr Iterator operator++(int) noexcept
            {
                Iterator copy{*this};
                ++(*this);
                return copy;
            }

            // LegacyBidirectionalIterator

            constexpr Iterator& operator--() noexcept
            {
                pos = pos == 0 ? N - 1 : pos - 1;
                return *this;
            }

            constexpr Iterator operator--(int) noexcept
            {
                Iterator copy{*this};
                --(*this);
                return copy;
            }

            // LegacyRandomAccessIterator

            constexpr Iterator& operator+=(difference_type i) noexcept
            {
                pos = (pos + i) % N;
                return *this;
            }

            constexpr Iterator operator+(difference_type i) const noexcept
            {
                Iterator copy{*this};
                copy += i;
                return copy;
            }

            constexpr Iterator& operator-=(difference_type i) noexcept
            {
                difference_type im = (i % N);

                if (im > pos)
                {
                    pos = N - (im - pos);
                }
                else
                {
                    pos = pos - im;
                }

                return *this;
            }

            constexpr Iterator operator-(difference_type i) const noexcept
            {
                Iterator copy{*this};
                copy -= i;
                return copy;
            }

            template<bool OtherConst>
            constexpr difference_type operator-(Iterator<OtherConst> const& other) const noexcept
            {
                return pos - other.pos;
            }

            template<bool _IsConst = IsConst>
            [[nodiscard]] constexpr typename std::enable_if_t<_IsConst, T const&> operator[](difference_type i) const noexcept
            {
                return data[(pos + i) % N];
            }

            template<bool _IsConst = IsConst>
            [[nodiscard]] constexpr typename std::enable_if_t<!_IsConst, T&> operator[](difference_type i) const noexcept
            {
                return data[(pos + i) % N];
            }

            template<bool OtherConst>
            constexpr bool operator<(Iterator<OtherConst> const& other) const noexcept
            {
                return pos < other.pos;
            }

            template<bool OtherConst>
            constexpr bool operator>(Iterator<OtherConst> const& other) const noexcept
            {
                return pos > other.pos;
            }

            template<bool OtherConst>
            constexpr bool operator>=(Iterator<OtherConst> const& other) const noexcept
            {
                return !(*this < other);
            }

            template<bool OtherConst>
            constexpr bool operator<=(Iterator<OtherConst> const& other) const noexcept
            {
                return !(*this > other);
            }
        };

    public:
        using value_type = T;
        using size_type = size_t;
        using reference = T&;
        using const_reference = T const&;
        using iterator = Iterator<false>;
        using const_iterator = Iterator<true>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        CircularBuffer() = default;
        CircularBuffer(CircularBuffer const&) = delete;
        CircularBuffer(CircularBuffer&&) noexcept = delete;
        CircularBuffer& operator=(CircularBuffer const&) = delete;
        CircularBuffer& operator=(CircularBuffer&&) noexcept = delete;
        ~CircularBuffer() noexcept
        {
            std::destroy(this->begin(), this->end());
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
            size_type idx = (static_cast<size_type>(m_Begin) + pos) % N;
            return *std::launder(reinterpret_cast<T*>(m_RawStorage.data() + idx));
        }

        [[nodiscard]] constexpr const_reference operator[](size_type pos) const noexcept {
            size_type idx = (static_cast<size_type>(m_Begin) + pos) % N;
            return *std::launder(reinterpret_cast<T const*>(m_RawStorage.data() + idx));
        }

        [[nodiscard]] constexpr reference front() noexcept {
            return *std::launder(reinterpret_cast<T*>(m_RawStorage.data() + static_cast<size_type>(m_Begin)));
        }

        [[nodiscard]] constexpr const_reference front() const noexcept {
            *std::launder(reinterpret_cast<T*>(m_RawStorage.data() + static_cast<size_type>(m_Begin)));
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
            T const* const_ptr = std::launder(reinterpret_cast<T const*>(m_RawStorage.data()));
            return const_iterator{const_cast<T*>(const_ptr), m_Begin};
        }

        [[nodiscard]] constexpr iterator begin() noexcept {
            T* ptr = std::launder(reinterpret_cast<T*>(m_RawStorage.data()));
            return iterator{ptr, m_Begin};
        }

        [[nodiscard]] constexpr const_iterator cbegin() const noexcept {
            return begin();
        }

        [[nodiscard]] constexpr const_iterator end() const noexcept {
            // the iterator is designed to handle const-ness
            T const* const_ptr = std::launder(reinterpret_cast<T const*>(m_RawStorage.data()));
            return const_iterator{const_cast<T*>(const_ptr), m_End};
        }

        [[nodiscard]] constexpr iterator end() noexcept {
            T* ptr = std::launder(reinterpret_cast<T*>(m_RawStorage.data()));
            return iterator{ptr, m_End};
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
            return m_Begin == m_End;
        }

        [[nodiscard]] constexpr size_type size() const noexcept {
            return m_End >= m_Begin ? m_End - m_Begin : (N - m_Begin) + m_End;
        }

        [[nodiscard]] constexpr size_type max_size() const noexcept {
            return N;
        }

        // modifiers
        //
        // be careful here: the data is type-punned from a sequence of bytes
        // so that the backing storage does not have a strict requirement of
        // having to contain redundant default-constrcuted elements

        constexpr void clear() noexcept
        {
            std::destroy(this->begin(), this->end());
            m_Begin = 0;
            m_End = 0;
        }

        template<typename... Args>
        constexpr T& emplace_back(Args&&... args)
        {
            ptrdiff_t new_end = (m_End + 1) % N;

            if (new_end == m_Begin)
            {
                // wraparound case: this is a fixed-size non-blocking circular
                // buffer
                //
                // there is a "dead" element in the buffer after the last element
                // but before the first (head). The head is about to become the
                // new "dead" element and should be destructed

                std::destroy_at(&front());
                m_Begin = (m_Begin + 1) % N;
            }

            // construct T in the old "dead" element location
            object_bytes* ptr = m_RawStorage.data() + m_End;
            T* constructed_el = new (ptr) T{std::forward<Args>(args)...};

            m_End = new_end;

            return *constructed_el;
        }

        constexpr void push_back(T const& v)
        {
            emplace_back(v);
        }

        constexpr void push_back(T&& v)
        {
            emplace_back(std::move(v));
        }

        constexpr iterator erase(iterator first, iterator last)
        {
            if (last != end())
            {
                throw std::runtime_error{"tried to remove a range of elements in the middle of a circular buffer (can currently only erase elements from end of circular buffer)"};
            }

            std::destroy(first, last);
            m_End -= std::distance(first, last);

            return end();
        }

        constexpr std::optional<T> try_pop_back()
        {
            std::optional<T> rv = std::nullopt;

            if (empty())
            {
                return rv;
            }

            rv.emplace(std::move(back()));
            m_End = m_End == 0 ? static_cast<ptrdiff_t>(N) - 1 : m_End - 1;

            return rv;
        }

        constexpr T pop_back()
        {
            if (empty())
            {
                throw std::runtime_error{"tried to call Circular_buffer::pop_back on an empty circular buffer"};
            }

            T rv{std::move(back())};
            m_End = m_End == 0 ? static_cast<ptrdiff_t>(N) - 1 : m_End - 1;
            return rv;
        }

        // raw (byte) storage for elements
        //
        // - it's raw bytes so that the implementation doesn't
        //   require a sequence of default-constructed Ts to
        //   populate the storage
        //
        // - the circular/modulo range [m_Begin..m_End) contains
        //   fully-constructed Ts
        //
        // - m_End always points to a "dead", but valid, location
        //   in storage
        //
        // - the above constraints imply that the number of "live"
        //   elements in storage is N-1, because m_End will modulo
        //   spin into position 0 once it is equal to N (this is
        //   in constrast to non-circular storage, where m_End
        //   typically points one past the end of the storage range)
        //
        // - this behavior makes the implementation simpler, because
        //   you don't have to handle m_Begin == m_End edge cases and
        //   one-past-the end out-of-bounds checks
        class alignas(T) object_bytes { std::byte data[sizeof(T)]; };
        std::array<object_bytes, N> m_RawStorage;

        // index (T-based, not raw byte based) of the first element
        ptrdiff_t m_Begin = 0;

        // first index (T-based, not raw byte based) *after* the last element
        ptrdiff_t m_End = 0;
    };
}

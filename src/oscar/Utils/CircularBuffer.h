#pragma once

#include <array>
#include <concepts>
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
    requires (N > 1)
    class CircularBuffer final {

        // iterator implementation
        template<bool IsConst>
        class Iterator final {
        public:
            using difference_type = ptrdiff_t;
            using value_type = T;
            using pointer = typename std::conditional_t<IsConst, const T*, T*>;
            using reference = typename std::conditional_t<IsConst, const T&, T&>;
            using iterator_category = std::random_access_iterator_tag;

            constexpr Iterator(pointer data, ptrdiff_t offset) :
                data_{data},
                offset_{offset}
            {}

            // implicit conversion from non-const iterator to a const one

            constexpr operator Iterator<true>() const
            {
                return Iterator<true>{data_, offset_};
            }

            // LegacyIterator

            constexpr Iterator& operator++()
            {
                offset_ = (offset_ + 1) % N;
                return *this;
            }

            constexpr reference operator*() const
            {
                return data_[offset_];
            }

            // EqualityComparable

            constexpr friend auto operator<=>(const Iterator&, const Iterator&) = default;

            // LegacyInputIterator

            constexpr pointer operator->() const
            {
                return &data_[offset_];
            }

            // LegacyForwardIterator

            constexpr Iterator operator++(int)
            {
                Iterator copy{*this};
                ++(*this);
                return copy;
            }

            // LegacyBidirectionalIterator

            constexpr Iterator& operator--()
            {
                offset_ = offset_ == 0 ? N - 1 : offset_ - 1;
                return *this;
            }

            constexpr Iterator operator--(int)
            {
                Iterator copy{*this};
                --(*this);
                return copy;
            }

            // LegacyRandomAccessIterator

            constexpr Iterator& operator+=(difference_type i)
            {
                offset_ = (offset_ + i) % N;
                return *this;
            }

            constexpr Iterator operator+(difference_type i) const
            {
                Iterator copy{*this};
                copy += i;
                return copy;
            }

            constexpr Iterator& operator-=(difference_type i)
            {
                difference_type im = (i % N);

                if (im > offset_)
                {
                    offset_ = N - (im - offset_);
                }
                else
                {
                    offset_ = offset_ - im;
                }

                return *this;
            }

            constexpr Iterator operator-(difference_type i) const
            {
                Iterator copy{*this};
                copy -= i;
                return copy;
            }

            constexpr difference_type operator-(const Iterator& other) const
            {
                return offset_ - other.offset_;
            }

            constexpr reference operator[](difference_type pos) const
            {
                return data_[(offset_ + pos) % N];
            }

        private:
            pointer data_ = nullptr;
            ptrdiff_t offset_ = 0;
        };

        Iterator(T*, ptrdiff_t) -> Iterator<false>;
        Iterator(const T*, ptrdiff_t) -> Iterator<true>;

    public:
        using value_type = T;
        using size_type = size_t;
        using reference = T&;
        using const_reference = const T&;
        using iterator = Iterator<false>;
        using const_iterator = Iterator<true>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        CircularBuffer() = default;
        CircularBuffer(const CircularBuffer&) = delete;
        CircularBuffer(CircularBuffer&&) noexcept = delete;
        CircularBuffer& operator=(const CircularBuffer&) = delete;
        CircularBuffer& operator=(CircularBuffer&&) noexcept = delete;
        ~CircularBuffer() noexcept
        {
            std::destroy(this->begin(), this->end());
        }


        // element access

        constexpr reference at(size_type pos) {
            return at(*this, pos);
        }

        constexpr const_reference at(size_type pos) const {
            return at(*this, pos);
        }

        constexpr reference operator[](size_type pos) {
            return at(*this, pos);
        }

        constexpr const_reference operator[](size_type pos) const {
            return at(*this, pos);
        }

        constexpr reference front() {
            return at(*this, 0);
        }

        constexpr const_reference front() const {
            return at(*this, 0);
        }

        constexpr reference back() {
            return *rbegin();
        }

        constexpr const_reference back() const {
            return *rbegin();
        }

        // iterators

        constexpr const_iterator begin() const {
            return iterator_at(*this, begin_offset_);
        }

        constexpr iterator begin() {
            return iterator_at(*this, begin_offset_);
        }

        constexpr const_iterator cbegin() const {
            return begin();
        }

        constexpr const_iterator end() const {
            return iterator_at(*this, end_offset_);
        }

        constexpr iterator end() {
            return iterator_at(*this, end_offset_);
        }

        constexpr const_iterator cend() const {
            return end();
        }

        constexpr const_reverse_iterator rbegin() const {
            return const_reverse_iterator{end()};
        }

        constexpr reverse_iterator rbegin() {
            return reverse_iterator{end()};
        }

        constexpr const_reverse_iterator crbegin() const {
            return const_reverse_iterator{end()};
        }

        constexpr const_reverse_iterator rend() const {
            return const_reverse_iterator{begin()};
        }

        constexpr reverse_iterator rend() {
            return reverse_iterator{begin()};
        }

        constexpr const_reverse_iterator crend() const {
            return const_reverse_iterator{begin()};
        }

        // capacity

        constexpr bool empty() const {
            return begin_offset_ == end_offset_;
        }

        constexpr size_type size() const {
            return end_offset_ >= begin_offset_ ? end_offset_ - begin_offset_ : (N - begin_offset_) + end_offset_;
        }

        constexpr size_type max_size() const {
            return N;
        }

        // modifiers
        //
        // be careful here: the data is type-punned from a sequence of bytes
        // so that the backing storage does not have a strict requirement of
        // having to contain redundant default-constrcuted elements

        constexpr void clear()
        {
            std::destroy(this->begin(), this->end());
            begin_offset_ = 0;
            end_offset_ = 0;
        }

        template<typename... Args>
        requires std::constructible_from<T, Args&&...>
        constexpr T& emplace_back(Args&&... args)
        {
            const ptrdiff_t new_end = (end_offset_ + 1) % N;

            if (new_end == begin_offset_) {
                // wraparound case: this is a fixed-size non-blocking circular
                // buffer
                //
                // there is a "dead" element in the buffer after the last element
                // but before the first (head). The head is about to become the
                // new "dead" element and should be destructed

                std::destroy_at(&front());
                begin_offset_ = (begin_offset_ + 1) % N;
            }

            // construct T in the old "dead" element location
            object_bytes* const ptr = raw_storage_bytes_.data() + end_offset_;
            T* const constructed_el = new (ptr) T{std::forward<Args>(args)...};

            end_offset_ = new_end;

            return *constructed_el;
        }

        constexpr void push_back(const T& value)
        {
            emplace_back(value);
        }

        constexpr void push_back(T&& value)
        {
            emplace_back(std::move(value));
        }

        constexpr iterator erase(iterator first, iterator last)
        {
            if (last != end()) {
                throw std::runtime_error{"tried to remove a range of elements in the middle of a circular buffer (can currently only erase elements from end of circular buffer)"};
            }

            std::destroy(first, last);
            end_offset_ -= std::distance(first, last);

            return end();
        }

        constexpr std::optional<T> try_pop_back()
        {
            std::optional<T> rv = std::nullopt;

            if (empty()) {
                return rv;
            }

            rv.emplace(std::move(back()));
            end_offset_ = end_offset_ == 0 ? static_cast<ptrdiff_t>(N) - 1 : end_offset_ - 1;

            return rv;
        }

        constexpr T pop_back()
        {
            if (empty()) {
                throw std::runtime_error{"tried to call Circular_buffer::pop_back on an empty circular buffer"};
            }

            T rv{std::move(back())};
            end_offset_ = end_offset_ == 0 ? static_cast<ptrdiff_t>(N) - 1 : end_offset_ - 1;
            return rv;
        }

    private:
        template<typename CircularBuf>
        static constexpr auto at(CircularBuf& buf, size_type pos) -> decltype(*buf.begin()) {
            if (pos >= buf.size()) {
                throw std::out_of_range{"tried to access a circular buffer element outside of its range"};
            }
            return buf.begin()[pos];
        }

        template<typename CircularBuf>
        static constexpr auto iterator_at(CircularBuf& buf, ptrdiff_t pos) {
            using Value = std::conditional_t<std::is_const_v<CircularBuf>, const T, T>;

            static_assert(alignof(Value) == alignof(typename decltype(buf.raw_storage_bytes_)::value_type));
            static_assert(sizeof(Value) == sizeof(typename decltype(buf.raw_storage_bytes_)::value_type));
            Value* ptr = std::launder(reinterpret_cast<Value*>(buf.raw_storage_bytes_.data()));  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

            return Iterator{ptr, pos};
        }

        // raw (byte) storage for elements
        //
        // - it's raw bytes so that the implementation doesn't
        //   require a sequence of default-constructed `T`s to
        //   populate the storage
        //
        // - the circular/modulo range `[begin_offset_..end_offset_)` contains
        //   fully-constructed `T`s
        //
        // - `end_offset_` always points to a "dead", but valid, location
        //   in storage
        //
        // - the above constraints imply that the number of "live"
        //   elements in storage is N-1, because `end_offset_` will modulo
        //   spin into position 0 once it is equal to `N` (this is
        //   in constrast to non-circular storage, where `end_offset_`
        //   typically points one past the end of the storage range)
        //
        // - this behavior makes the implementation simpler, because
        //   you don't have to handle `begin_offset_ == end_offset_` edge
        //   cases and one-past-the end out-of-bounds checks
        class alignas(T) object_bytes { std::byte data[sizeof(T)]; };
        std::array<object_bytes, N> raw_storage_bytes_;

        // index (`T`-based, not raw byte based) of the first element
        ptrdiff_t begin_offset_ = 0;

        // first index (`T`-based, not raw byte based) *after* the last element
        ptrdiff_t end_offset_ = 0;
    };
}

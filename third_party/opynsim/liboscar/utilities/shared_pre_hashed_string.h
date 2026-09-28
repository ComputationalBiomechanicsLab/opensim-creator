#pragma once

#include <liboscar/utilities/c_string_view.h>

#include <atomic>
#include <compare>
#include <concepts>
#include <functional>
#include <memory>
#include <new>
#include <ostream>
#include <string_view>
#include <utility>

namespace osc
{
    // an immutable, reference-counted, and pre-hashed string-like object
    //
    // key differences to `std::string`:
    //
    // + only one pointer in size (`std::string` tends to be 3 pointers or so)
    // + cheap to copy
    // + cheap to equality-compare copies
    // + cheap to `std::hash` (as it's pre-hashed)
    // ~ immutable
    // ~ one branch more expensive than `std::string` to compare when not comparing a copy
    // - always heap-allocates its representation (i.e. no SSO)
    // - always pre-hashes the string content, even if you don't plan on using the hash
    //
    // key differences to `StringName`:
    //
    // + doesn't a global lookup
    // - because there's no global deduplication, duplication of string data across
    //   separately-constructed instances is possible
    //
    // usage recommendations:
    //
    // - use `std::string` / `std::string_view` for almost all day-to-day string
    //   use-cases, especially if the strings are likely to be short enough for
    //   SSO and you don't plan on (re)hashing the string much
    //
    // - use `SharedPreHashedString` in systems that are mostly isolated in one place,
    //   use, copy, and hash a lot of potentially-longer strings in associative lookups.
    //   e.g. a standalone system that reads potentially-heavily-repeated strings from
    //   an input file, where the system could reasonably have its own
    //   `std::unordered_set<SharedPreHashedString>` or similar
    //
    // - use `StringName` in larger multi-level systems that use, copy, and hash a lot of
    //   potentially-longer strings in associative lookups. E.g. a large application
    //   containing a graphics system, which binds material keys to shader uniforms on the GPU;
    //   a mid-level UI controller, which maintains a datamodel containing material key-value
    //   pairs; and a high-level UI, which binds those pairs to a nodegraph UI (i.e. despite
    //   each tier having an independent responsibility, there's likely to be overlaps in
    //   data values)
    class SharedPreHashedString {
    public:
        using traits_type = std::string_view::traits_type;
        using value_type = std::string_view::value_type;
        using size_type = std::string_view::size_type;
        using difference_type = std::string_view::difference_type;
        using reference = std::string_view::const_reference;
        using const_reference = std::string_view::const_reference;
        using iterator = std::string_view::const_iterator;
        using const_iterator = std::string_view::const_iterator;
        using reverse_iterator = std::string_view::const_reverse_iterator;
        using const_reverse_iterator = std::string_view::const_reverse_iterator;

        explicit SharedPreHashedString() :
            SharedPreHashedString{static_default_instance()}
        {}
        explicit SharedPreHashedString(std::string_view str) :
            storage_{Storage::create(str)}
        {}
        explicit SharedPreHashedString(const char* s) : SharedPreHashedString{std::string_view{s}} {}
        explicit SharedPreHashedString(std::nullptr_t) = delete;

        SharedPreHashedString(const SharedPreHashedString& src) noexcept :
            storage_{src.storage_}
        {
            storage_->num_owners.fetch_add(1, std::memory_order_relaxed);
        }

        SharedPreHashedString(SharedPreHashedString&& tmp) noexcept :
            storage_{tmp.storage_}
        {
            storage_->num_owners.fetch_add(1, std::memory_order_relaxed);
        }

        SharedPreHashedString& operator=(const SharedPreHashedString& src) noexcept
        {
            SharedPreHashedString copy{src};
            swap(*this, copy);
            return *this;
        }

        SharedPreHashedString& operator=(SharedPreHashedString&& tmp) noexcept
        {
            swap(*this, tmp);
            return *this;
        }

        ~SharedPreHashedString() noexcept
        {
            if (storage_->num_owners.fetch_sub(1, std::memory_order_release) == 1) {
                std::atomic_thread_fence(std::memory_order_acquire);
                delete storage_;
            }
        }

        friend void swap(SharedPreHashedString& a, SharedPreHashedString& b) noexcept
        {
            std::swap(a.storage_, b.storage_);
        }

        // returns a reference to the character at specified position `pos` if `pos <= size()`. Bounds checking
        // is performed, and an exception of type `std::out_of_range` will be thrown on invalid access
        const_reference at(size_type pos) const
        {
            return std::string_view{*this}.at(pos);
        }

        // returns a reference to the character at the specified position `pos` if `pos < size()`, or a
        // reference to `value_type()` if `pos == size()`. No bounds checking is performed. If `pos > size()`
        // the behavior is undefined
        const_reference operator[](size_type pos) const
        {
            return data()[pos];
        }

        // returns a reference to the first character in the string. The behavior is undefined if `empty()` is `true`
        const_reference front() const
        {
            return *data();
        }

        // returns a reference to the last character in the string. The behavior is undefined if `empty()` is `true`
        const_reference back() const
        {
            return std::string_view{*this}.back();
        }

        // returns a pointer to the underlying array serving as character storage
        const value_type* data() const
        {
            return storage_->data();
        }

        // returns a pointer to a null-terminated character array with data equivalent to those stored in the string.
        const value_type* c_str() const
        {
            return data();
        }

        // returns a non-modifiable `std::string_view` of the string's data
        operator std::string_view() const noexcept
        {
            return std::string_view{data(), size()};
        }

        // returns a non-modifiable `CStringView` of the string's data
        operator CStringView() const noexcept
        {
            return CStringView{data(), size()};
        }

        // returns an iterator to the first character in the string
        const_iterator begin() const noexcept
        {
            return std::string_view{*this}.begin();
        }

        // returns a constant iterator to the first character in the string
        const_iterator cbegin() const noexcept
        {
            return std::string_view{*this}.cbegin();
        }

        // returns an iterator to the character following the last character of the string
        const_iterator end() const noexcept
        {
            return std::string_view{*this}.end();
        }

        // returns a constant iterator to the character following the last character of the string
        const_iterator cend() const noexcept
        {
            return std::string_view{*this}.cend();
        }

        // returns `true` if the string contains no characters
        [[nodiscard]] bool empty() const noexcept
        {
            return size() == 0;
        }

        bool starts_with(std::string_view sv) const noexcept
        {
            return std::string_view{*this}.starts_with(sv);
        }

        // returns the number of characters in the string
        size_type size() const
        {
            return storage_->size;
        }

        friend constexpr bool operator==(const SharedPreHashedString& lhs, const SharedPreHashedString& rhs)
        {
            return lhs.storage_ == rhs.storage_ or std::string_view{lhs} == std::string_view{rhs};
        }

        template<typename StringViewLike>
        requires std::constructible_from<std::string_view, StringViewLike>
        friend constexpr bool operator==(const SharedPreHashedString& lhs, const StringViewLike& rhs)
        {
            return std::string_view{lhs} == rhs;
        }

        template<typename StringViewLike>
        requires std::constructible_from<std::string_view, StringViewLike>
        friend constexpr bool operator==(const StringViewLike& lhs, const SharedPreHashedString& rhs)
        {
            return lhs == std::string_view{rhs};
        }

        friend constexpr auto operator<=>(const SharedPreHashedString& lhs, const SharedPreHashedString& rhs)
        {
            if (lhs.storage_ == rhs.storage_) {
                return std::strong_ordering::equal;
            }
            return std::string_view{lhs} <=> std::string_view{rhs};
        }

        template<typename StringViewLike>
        requires std::constructible_from<std::string_view, StringViewLike>
        friend constexpr auto operator<=>(const SharedPreHashedString& lhs, const StringViewLike& rhs)
        {
            return std::string_view{lhs} <=> rhs;
        }

        template<typename StringViewLike>
        requires std::constructible_from<std::string_view, StringViewLike>
        friend constexpr auto operator<=>(const StringViewLike& lhs, const SharedPreHashedString& rhs)
        {
            return lhs <=> std::string_view{rhs};
        }

        friend std::ostream& operator<<(std::ostream& lhs, const SharedPreHashedString& rhs)
        {
            return lhs << std::string_view{rhs};
        }

        // returns the number of different `SharedPreHashedString` instances (`this` included)
        // managing the same underlying string data. In a multithreaded environment, the value
        // returned by `use_count` is approximate (i.e. the implementation uses a
        // `std::memory_order_relaxed` operation to load the count)
        long use_count() const noexcept
        {
            return static_cast<long>(storage_->num_owners.load(std::memory_order_relaxed));
        }

        size_t hash() const
        {
            return storage_->hash;
        }
    private:
        static const SharedPreHashedString& static_default_instance()
        {
            static SharedPreHashedString s_default_instance{std::string_view{}};
            return s_default_instance;
        }

        // Internal storage for this string type. Stores metadata (size, hash, etc.)
        // immediately followed by the string's bytes.
        class Storage final {
        public:
            static Storage* create(std::string_view str)
            {
                // Allocate memory
                void* mem = ::operator new(storage_allocation_size(str.size()), std::align_val_t{alignof(Storage)});

                // Initialize header at the front
                std::unique_ptr<Storage> storage{::new (mem) Storage{str}};

                // Copy string data after it.
                std::uninitialized_copy_n(str.data(), str.size(), storage->data());

                // Initialize NUL terminator after string data.
                storage->data()[str.size()] = value_type{};

                return storage.release();
            }

            Storage(const Storage&) = delete;
            Storage(Storage&&) noexcept = delete;
            Storage& operator=(const Storage&) = delete;
            Storage& operator=(Storage&&) noexcept = delete;
            ~Storage() noexcept = default;

            void operator delete(Storage* ptr, std::destroying_delete_t)
            {
                ptr->~Storage();
                ::operator delete(
                    static_cast<void*>(ptr),
                    storage_allocation_size(ptr->size),
                    std::align_val_t{alignof(Storage)}
                );
            }

            value_type* data() noexcept
            {
                static_assert(alignof(Storage) >= alignof(value_type));
                return reinterpret_cast<value_type*>(this + 1);
            }
            const value_type* data() const noexcept
            {
                static_assert(alignof(Storage) >= alignof(value_type));
                return reinterpret_cast<const value_type*>(this + 1);
            }

            size_t size;
            size_t hash;
            std::atomic<size_t> num_owners = 1;

        private:
            static constexpr size_t storage_allocation_size(size_t string_len)
            {
                // Header + data + NUL
                static_assert(alignof(Storage) >= alignof(value_type));
                return sizeof(Storage) + (sizeof(value_type)*(string_len + 1));
            }

            explicit Storage(std::string_view str) noexcept :
                size{str.size()},
                hash{std::hash<std::string_view>{}(str)}
            {}
        };

        Storage* storage_;
    };
}

template<>
struct std::hash<osc::SharedPreHashedString> final {
    size_t operator()(const osc::SharedPreHashedString& str) const noexcept
    {
        return str.hash();
    }
};

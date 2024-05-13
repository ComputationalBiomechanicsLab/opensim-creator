#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iosfwd>

namespace osc
{
    // Unique ID
    //
    // an ID that is guaranteed to be unique upon non-copy/move construction
    class UID final {
    public:
        using element_type = intptr_t;

        static constexpr UID invalid()
        {
            return UID{-1};
        }

        static constexpr UID empty()
        {
            return UID{0};
        }

        static constexpr UID from_int_unchecked(element_type i)
        {
            return UID{i};
        }

        UID() : value_{allocate_next_id()}
        {}
        constexpr UID(const UID&) = default;
        constexpr UID(UID&&) noexcept = default;
        constexpr UID& operator=(const UID&) = default;
        constexpr UID& operator=(UID&&) noexcept = default;
        ~UID() noexcept = default;

        void reset()
        {
            value_ = allocate_next_id();
        }

        constexpr element_type get() const
        {
            return value_;
        }

        explicit constexpr operator bool() const
        {
            return value_ > 0;
        }

        friend auto operator<=>(const UID&, const UID&) = default;

    private:
        static constinit std::atomic<element_type> g_next_available_id;

        static element_type allocate_next_id()
        {
            return g_next_available_id.fetch_add(1, std::memory_order_relaxed);
        }

        constexpr UID(element_type value) : value_{value}
        {}

        element_type value_;
    };

    std::ostream& operator<<(std::ostream&, const UID&);
}

// hashing support for LogicalIDs
//
// lets them be used as associative lookup keys, etc.
template<>
struct std::hash<osc::UID> final {
    size_t operator()(const osc::UID& id) const
    {
        return static_cast<size_t>(id.get());
    }
};

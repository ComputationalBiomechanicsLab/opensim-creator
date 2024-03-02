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

        static constexpr UID FromIntUnchecked(element_type i)
        {
            return UID{i};
        }

        UID() : m_Value{GetNextID()}
        {
        }
        constexpr UID(UID const&) = default;
        constexpr UID(UID&&) noexcept = default;
        constexpr UID& operator=(UID const&) = default;
        constexpr UID& operator=(UID&&) noexcept = default;
        ~UID() noexcept = default;

        void reset()
        {
            m_Value = GetNextID();
        }

        constexpr element_type get() const
        {
            return m_Value;
        }

        explicit constexpr operator bool() const
        {
            return m_Value > 0;
        }

        friend auto operator<=>(UID const&, UID const&) = default;

    private:
        static constinit std::atomic<element_type> g_NextID;
        static element_type GetNextID()
        {
            return g_NextID.fetch_add(1, std::memory_order_relaxed);
        }

        constexpr UID(element_type value) : m_Value{value}
        {
        }

        element_type m_Value;
    };

    std::ostream& operator<<(std::ostream&, UID const&);
}

// hashing support for LogicalIDs
//
// lets them be used as associative lookup keys, etc.
template<>
struct std::hash<osc::UID> final {
    size_t operator()(osc::UID const& id) const
    {
        return static_cast<size_t>(id.get());
    }
};

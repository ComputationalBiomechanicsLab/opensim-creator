#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iosfwd>

namespace osc
{
    class UID {
        friend UID GenerateID() noexcept;
        friend constexpr int64_t UnwrapID(UID const&) noexcept;

    protected:
        explicit constexpr UID(int64_t value) noexcept :
            m_Value{value}
        {
        }

    private:
        int64_t m_Value;
    };

    // strongly-typed version of the above
    //
    // adds compile-time type checking to IDs
    template<typename T>
    class UIDT : public UID {
        template<typename U>
        friend UIDT<U> GenerateIDT() noexcept;

        template<typename U>
        friend constexpr UIDT<U> DowncastID(UID const&) noexcept;

    private:
        explicit constexpr UIDT(UID id) :
            UID{id}
        {
        }
    };

    extern std::atomic<int64_t> g_NextGlobalUID;

    inline UID GenerateID() noexcept
    {
        return UID{g_NextGlobalUID.fetch_add(1, std::memory_order_relaxed)};
    }

    template<typename T>
    UIDT<T> GenerateIDT() noexcept
    {
        return UIDT<T>{GenerateID()};
    }

    constexpr int64_t UnwrapID(UID const& id) noexcept
    {
        return id.m_Value;
    }

    std::ostream& operator<<(std::ostream& o, UID const& id);

    constexpr bool operator==(UID const& lhs, UID const& rhs) noexcept
    {
        return UnwrapID(lhs) == UnwrapID(rhs);
    }

    constexpr bool operator!=(UID const& lhs, UID const& rhs) noexcept
    {
        return UnwrapID(lhs) != UnwrapID(rhs);
    }

    constexpr bool operator<(UID const& lhs, UID const& rhs) noexcept
    {
        return UnwrapID(lhs) < UnwrapID(rhs);
    }

    template<typename T>
    constexpr UIDT<T> DowncastID(UID const& id) noexcept
    {
        return UIDT<T>{id};
    }
}

// hashing support for LogicalIDs
//
// lets them be used as associative lookup keys, etc.
namespace std
{
    template<>
    struct hash<osc::UID> {
        size_t operator()(osc::UID const& id) const
        {
            return static_cast<size_t>(osc::UnwrapID(id));
        }
    };

    template<typename T>
    struct hash<osc::UIDT<T>> {
        size_t operator()(osc::UID const& id) const
        {
            return static_cast<size_t>(osc::UnwrapID(id));
        }
    };
}

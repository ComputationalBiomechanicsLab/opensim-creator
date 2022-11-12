#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <type_traits>
#include <utility>

namespace osc
{
    class UID {
    public:
        static constexpr UID invalid() noexcept
        {
            return UID{-1};
        }

        static constexpr UID empty() noexcept
        {
            return UID{0};
        }

        UID() noexcept : m_Value{GetNextID()}
        {
        }
        constexpr UID(UID const&) = default;
        constexpr UID(UID&&) noexcept = default;
        constexpr UID& operator=(UID const&) = default;
        constexpr UID& operator=(UID&&) noexcept = default;
        ~UID() noexcept = default;

        void reset() noexcept
        {
            m_Value = GetNextID();
        }

        constexpr int64_t get() const noexcept
        {
            return m_Value;
        }

        explicit constexpr operator bool() const noexcept
        {
            return m_Value > 0;
        }

    private:
        static int64_t GetNextID() noexcept;

        constexpr UID(int64_t value) noexcept : m_Value{std::move(value)}
        {
        }

        int64_t m_Value;
    };

    // strongly-typed version of the above
    //
    // adds compile-time type checking to IDs
    template<typename T>
    class UIDT : public UID {
    public:

        UIDT() : UID{}
        {
        }

        // upcasting is automatic
        template<typename U, typename = std::enable_if_t<std::is_base_of_v<U, T>>>
        explicit constexpr operator UIDT<U> () const noexcept
        {
            return UIDT<U>{*this};
        }

    private:
        // unchecked downcast of untyped ID to typed one
        template<typename U>
        friend constexpr UIDT<U> DowncastID(UID const&) noexcept;

        constexpr UIDT(UID id) noexcept : UID{std::move(id)}
        {
        }
    };

    template<typename U>
    constexpr UIDT<U> DowncastID(UID const& id) noexcept
    {
        return UIDT<U>{id};
    }

    template<typename U, typename V>
    constexpr UIDT<V> DowncastIDT(UIDT<U> const& id) noexcept
    {
        static_assert(std::is_base_of_v<V, U>);
        return UIDT<V>{id};
    }

    std::ostream& operator<<(std::ostream& o, UID const& id);

    constexpr bool operator==(UID const& lhs, UID const& rhs) noexcept
    {
        return lhs.get() == rhs.get();
    }

    constexpr bool operator!=(UID const& lhs, UID const& rhs) noexcept
    {
        return lhs.get() != rhs.get();
    }

    constexpr bool operator<(UID const& lhs, UID const& rhs) noexcept
    {
        return lhs.get() < rhs.get();
    }

    constexpr bool operator<=(UID const& lhs, UID const& rhs) noexcept
    {
        return lhs.get() <= rhs.get();
    }

    constexpr bool operator>(UID const& lhs, UID const& rhs) noexcept
    {
        return lhs.get() > rhs.get();
    }

    constexpr bool operator>=(UID const& lhs, UID const& rhs) noexcept
    {
        return lhs.get() >= rhs.get();
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
            return static_cast<size_t>(id.get());
        }
    };

    template<typename T>
    struct hash<osc::UIDT<T>> {
        size_t operator()(osc::UID const& id) const
        {
            return static_cast<size_t>(id.get());
        }
    };
}

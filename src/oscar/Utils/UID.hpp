#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iosfwd>

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

        friend auto operator<=>(UID const&, UID const&) = default;

    private:
        static int64_t GetNextID() noexcept;

        constexpr UID(int64_t value) noexcept : m_Value{value}
        {
        }

        int64_t m_Value;
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

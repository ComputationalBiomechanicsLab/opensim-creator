#pragma once

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
        static constexpr UID invalid()
        {
            return UID{-1};
        }

        static constexpr UID empty()
        {
            return UID{0};
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

        constexpr int64_t get() const
        {
            return m_Value;
        }

        explicit constexpr operator bool() const
        {
            return m_Value > 0;
        }

        friend auto operator<=>(UID const&, UID const&) = default;

    private:
        static int64_t GetNextID();

        constexpr UID(int64_t value) : m_Value{value}
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

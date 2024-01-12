#pragma once

#include <oscar/Utils/Concepts.hpp>

#include <utility>
#include <type_traits>

namespace osc
{
    template<typename T>
    class DefaultConstructOnCopy final {
    public:
        static_assert(std::is_default_constructible_v<T>);

        DefaultConstructOnCopy() = default;

        template<typename... Args>
        DefaultConstructOnCopy(Args&& ...args)
            requires ConstructibleFrom<T, Args&&...> :

            m_Value{std::forward<Args>(args)...}
        {
        }

        DefaultConstructOnCopy(DefaultConstructOnCopy const&) : m_Value{}
        {
        }

        DefaultConstructOnCopy(DefaultConstructOnCopy&&) noexcept = default;

        DefaultConstructOnCopy& operator=(DefaultConstructOnCopy const&)
        {
            m_Value = T{};  // exception safety: construct it then move-assign it
            return *this;
        }

        DefaultConstructOnCopy& operator=(DefaultConstructOnCopy&&) noexcept = default;

        ~DefaultConstructOnCopy() noexcept = default;

        T* operator->() { return &m_Value; }
        T const* operator->() const { return &m_Value; }
        T& operator*() { return m_Value; }
        T const& operator*() const { return m_Value; }
        T* get() { return m_Value; }
        T const* get() const { return m_Value; }

        void reset() { m_Value = T{}; }

    private:
        T m_Value;
    };
}

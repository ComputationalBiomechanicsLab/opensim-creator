#pragma once

#include <concepts>
#include <utility>

namespace osc
{
    template<std::default_initializable T>
    class DefaultConstructOnCopy final {
    public:
        DefaultConstructOnCopy() = default;

        template<typename... Args>
        requires std::constructible_from<T, Args&&...>
        DefaultConstructOnCopy(Args&& ...args) :
            m_Value{std::forward<Args>(args)...}
        {}

        DefaultConstructOnCopy(const DefaultConstructOnCopy&) :
            m_Value{}
        {}

        DefaultConstructOnCopy(DefaultConstructOnCopy&&) noexcept = default;

        DefaultConstructOnCopy& operator=(const DefaultConstructOnCopy&)
        {
            m_Value = T{};  // exception safety: construct it then move-assign it
            return *this;
        }

        DefaultConstructOnCopy& operator=(DefaultConstructOnCopy&&) noexcept = default;

        ~DefaultConstructOnCopy() noexcept = default;

        T* operator->() { return &m_Value; }
        const T* operator->() const { return &m_Value; }
        T& operator*() { return m_Value; }
        const T& operator*() const { return m_Value; }
        T* get() { return m_Value; }
        const T* get() const { return m_Value; }

        void reset() { m_Value = T{}; }

    private:
        T m_Value;
    };
}

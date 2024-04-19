#pragma once

#include <concepts>
#include <utility>

namespace osc
{
    template<std::invocable Dtor>
    class ScopeGuard final {
    public:
        explicit ScopeGuard(Dtor&& dtor_) :
            m_OnScopeExit{std::forward<Dtor&&>(dtor_)}
        {}
        ScopeGuard(const ScopeGuard&) = delete;
        ScopeGuard(ScopeGuard&&) noexcept = delete;
        ScopeGuard& operator=(const ScopeGuard&) = delete;
        ScopeGuard& operator=(ScopeGuard&&) noexcept = delete;
        ~ScopeGuard() noexcept(noexcept(m_OnScopeExit()))
        {
            m_OnScopeExit();
        }

    private:
        Dtor m_OnScopeExit;
    };
}

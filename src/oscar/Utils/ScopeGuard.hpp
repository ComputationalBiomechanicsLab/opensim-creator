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
        {
        }
        ScopeGuard(ScopeGuard const&) = delete;
        ScopeGuard(ScopeGuard&&) noexcept = delete;
        ScopeGuard& operator=(ScopeGuard const&) = delete;
        ScopeGuard& operator=(ScopeGuard&&) noexcept = delete;
        ~ScopeGuard() noexcept(noexcept(m_OnScopeExit()))
        {
            m_OnScopeExit();
        }

    private:
        Dtor m_OnScopeExit;
    };
}

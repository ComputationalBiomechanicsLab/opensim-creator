#pragma once

#include <utility>

namespace osc
{
    template<typename Dtor>
    class ScopeGuard final {
    public:
        explicit ScopeGuard(Dtor&& dtor_) noexcept :
            m_OnScopeExit{std::forward<Dtor&&>(dtor_)}
        {
        }
        ScopeGuard(ScopeGuard const&) = delete;
        ScopeGuard(ScopeGuard&&) noexcept = delete;
        ScopeGuard& operator=(ScopeGuard const&) = delete;
        ScopeGuard& operator=(ScopeGuard&&) noexcept = delete;
        ~ScopeGuard() noexcept
        {
            m_OnScopeExit();
        }

    private:
        Dtor m_OnScopeExit;
    };
}

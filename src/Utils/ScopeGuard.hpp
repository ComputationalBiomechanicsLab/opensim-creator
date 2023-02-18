#pragma once

#include "src/Utils/Macros.hpp"

#include <utility>

namespace osc
{
    template<typename Dtor>
    class ScopeGuard final {
    public:
        ScopeGuard(Dtor&& dtor_) noexcept :
            m_OnScopeExit{std::move(dtor_)}
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

#define OSC_SCOPE_GUARD(action) osc::ScopeGuard OSC_TOKENPASTE2(guard_, __LINE__){[&]() action};

#define OSC_SCOPE_GUARD_IF(cond, action) OSC_SCOPE_GUARD({ if (cond) { action } })

#pragma once

#include <concepts>
#include <utility>

namespace osc
{
    template<std::invocable Dtor>
    class ScopeGuard final {
    public:
        explicit ScopeGuard(Dtor&& destructor) :
            destructor_{std::forward<Dtor&&>(destructor)}
        {}
        ScopeGuard(const ScopeGuard&) = delete;
        ScopeGuard(ScopeGuard&&) noexcept = delete;
        ScopeGuard& operator=(const ScopeGuard&) = delete;
        ScopeGuard& operator=(ScopeGuard&&) noexcept = delete;
        ~ScopeGuard() noexcept(noexcept(destructor_()))
        {
            destructor_();
        }

    private:
        Dtor destructor_;
    };
}

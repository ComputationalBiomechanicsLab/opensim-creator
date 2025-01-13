#pragma once

#include <concepts>
#include <utility>

namespace osc
{
    template<std::invocable Destructor>
    class ScopeGuard final {
    public:
        explicit ScopeGuard(Destructor&& destructor) :
            destructor_{std::forward<Destructor>(destructor)}
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
        Destructor destructor_;
    };
}

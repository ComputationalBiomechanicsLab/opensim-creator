#pragma once

#include <concepts>
#include <utility>

namespace osc
{
    // A general-purpose scope guard that calls its exit function when
    // a scope is exited (either normally, or via an exception).
    //
    // Inspired by `std::experimental::scope_exit`.
    template<std::invocable Destructor>
    class [[nodiscard]] ScopeExit final {
    public:
        explicit ScopeExit(Destructor&& exit_function) :
            exit_function_{std::forward<Destructor>(exit_function)}
        {}
        ScopeExit(const ScopeExit&) = delete;
        ScopeExit(ScopeExit&&) noexcept = delete;
        ScopeExit& operator=(const ScopeExit&) = delete;
        ScopeExit& operator=(ScopeExit&&) noexcept = delete;
        ~ScopeExit() noexcept(noexcept(exit_function_()))
        {
            if (is_active_) {
                exit_function_();
            }
        }

        void release() noexcept { is_active_ = false; }

    private:
        Destructor exit_function_;
        bool is_active_ = true;
    };
}

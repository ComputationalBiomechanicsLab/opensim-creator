#pragma once

#include <concepts>
#include <type_traits>
#include <utility>

namespace osc
{
    /// A general-purpose scope guard intended to call its exit function when
    /// a scope is exited - either normally, or via an exception.
    ///
    /// This utility class is effectively an OpenSim rewrite of `std::experimental::scope_exit`,
    /// which is documented here: https://en.cppreference.com/w/cpp/experimental/scope_exit.html
    template<std::invocable EF>
    class [[nodiscard]] ScopeExit final {
    public:
        /// Constructs a `ScopeExit` from a function or function object.
        template<typename Fn>
        requires (
            std::destructible<EF> and
            std::is_constructible_v<EF, Fn> and
            not std::same_as<std::remove_cvref_t<Fn>, ScopeExit>
        )
        explicit ScopeExit(Fn&& fn) : exit_function_{std::forward<Fn>(fn)} {}

        ScopeExit(const ScopeExit&) = delete;

        /// Move constructor. Initializes the stored function with the one in
        /// `tmp`. The constructed `ScopeExit` is active if and only if `tmp`
        /// is active. After successful move construction, `tmp` becomes
        /// inactive.
        ScopeExit(ScopeExit&& tmp) noexcept
        requires (std::is_nothrow_move_constructible_v<EF>) :
            exit_function_{std::move(tmp.exit_function_)},
            is_active_{std::exchange(tmp.is_active_, false)}
        {}

        /// \copydoc ScopeExit::ScopeExit(ScopeExit&&)
        ScopeExit(ScopeExit&& tmp) noexcept(std::is_nothrow_copy_constructible_v<EF>)
        requires (not std::is_nothrow_move_constructible_v<EF> and std::is_copy_constructible_v<EF>) :
            exit_function_{tmp.exit_function_},
            is_active_{std::exchange(tmp.is_active_, false)}
        {}

        ScopeExit& operator=(const ScopeExit&) = delete;
        ScopeExit& operator=(ScopeExit&&) noexcept = delete;

        ~ScopeExit() noexcept
        {
            if (is_active_) {
                exit_function_();
            }
        }

        /// Makes the `ScopeExit` inactive, meaning it will not call its
        /// exit function on destruction.
        ///
        /// Once a `ScopeExit` is inactive, it cannot become active again.
        void release() noexcept { is_active_ = false; }

        private:
            EF exit_function_;
            bool is_active_ = true;
    };

    /// One template deduction guide permits the deduction of an argument of function
    /// or function object type.
    ///
    /// The argument (after function-to-pointer decay, if any) is copied or moved into
    /// the constructed scope_exit.
    template<typename EF>
    ScopeExit(EF) -> ScopeExit<EF>;
}

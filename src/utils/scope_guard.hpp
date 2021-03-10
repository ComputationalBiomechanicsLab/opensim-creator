#pragma once

#include <utility>

namespace osmv {
    template<typename Dtor>
    class Scope_guard final {
        Dtor dtor;

    public:
        Scope_guard(Dtor&& _dtor) noexcept : dtor{std::move(_dtor)} {
        }
        Scope_guard(Scope_guard const&) = delete;
        Scope_guard(Scope_guard&&) = delete;
        Scope_guard& operator=(Scope_guard const&) = delete;
        Scope_guard& operator=(Scope_guard&&) = delete;
        ~Scope_guard() noexcept {
            dtor();
        }
    };
}

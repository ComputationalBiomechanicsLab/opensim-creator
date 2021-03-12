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

#define OSMV_TOKENPASTE(x, y) x##y
#define OSMV_TOKENPASTE2(x, y) OSMV_TOKENPASTE(x, y)
#define OSMV_SCOPE_GUARD(action) Scope_guard OSMV_TOKENPASTE2(guard_, __LINE__){[&]() action};
#define OSMV_SCOPE_GUARD_IF(cond, action)                                                                              \
    OSMV_SCOPE_GUARD({                                                                                                 \
        if (cond) {                                                                                                    \
            action                                                                                                     \
        }                                                                                                              \
    })
}

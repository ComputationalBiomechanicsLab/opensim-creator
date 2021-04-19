#pragma once

#include <utility>

namespace osc {
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

#define OSC_TOKENPASTE(x, y) x##y
#define OSC_TOKENPASTE2(x, y) OSC_TOKENPASTE(x, y)
#define OSC_SCOPE_GUARD(action) osc::Scope_guard OSC_TOKENPASTE2(guard_, __LINE__){[&]() action};
#define OSC_SCOPE_GUARD_IF(cond, action)                                                                              \
    OSC_SCOPE_GUARD({                                                                                                 \
        if (cond) {                                                                                                    \
            action                                                                                                     \
        }                                                                                                              \
    })
}

#pragma once

#include <memory>

namespace osc
{
    class SharedLifetimeBlock;

    // a non-owning reference to a lifetime block that can be queried at
    // runtime (via `expired`) to check if the lifetime is still alive
    class LifetimeWatcher final {
    public:
        constexpr LifetimeWatcher() = default;

        bool expired() const noexcept { return ptr_.expired(); }
    private:
        friend class SharedLifetimeBlock;
        explicit LifetimeWatcher(const std::shared_ptr<int>& ptr) :
            ptr_{ptr}
        {}

        std::weak_ptr<int> ptr_;
    };
}

#pragma once

#include <oscar/Utils/LifetimeWatcher.h>

#include <memory>

namespace osc
{
    // a managed lifetime that may have multiple owners and non-owning watchers
    class SharedLifetimeBlock final {
    public:
        LifetimeWatcher watch() const { return LifetimeWatcher{ptr_}; }
        size_t num_owners() const { return static_cast<size_t>(ptr_.use_count()); }
    private:
        std::shared_ptr<int> ptr_ = std::make_shared<int>();
    };
}

#pragma once

#include <oscar/Utils/LifetimeWatcher.h>
#include <oscar/Utils/SharedLifetimeBlock.h>

namespace osc
{
    // a managed, reference-counted, watchable lifetime that's strongly tied
    // to its location in memory (scope) - to the point that copying, moving,
    // or assigning it resets the lifetime
    class ScopedLifetime final {
    public:
        // constructs a new lifetime
        ScopedLifetime() = default;

        // constructs a new lifetime
        ScopedLifetime(const ScopedLifetime&) : ScopedLifetime{} {}

        // replaces the lifetime with a new lifetime
        ScopedLifetime& operator=(const ScopedLifetime& rhs)
        {
            if (&rhs == this) {
                return *this;
            }

            lifetime_block_ = SharedLifetimeBlock{};
            return *this;
        }

        ~ScopedLifetime() noexcept = default;

        LifetimeWatcher watch() const { return lifetime_block_.watch(); }
    private:
        SharedLifetimeBlock lifetime_block_;
    };
}

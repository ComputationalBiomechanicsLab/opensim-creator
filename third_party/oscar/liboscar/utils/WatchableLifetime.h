#pragma once

#include <liboscar/utils/LifetimeWatcher.h>

#include <concepts>

namespace osc
{
    // satisfied if `T` is an object that can have its lifetime watched
    template<typename T>
    concept WatchableLifetime = requires(const T& lifetime)
    {
        { lifetime.watch() } -> std::convertible_to<LifetimeWatcher>;
    };
}

#include <oscar/Utils/ScopedLifetime.h>

#include <oscar/Utils/LifetimeWatcher.h>
#include <oscar/Utils/WatchableLifetime.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(ScopedLifetime, satisfies_WatchableLifetime_concept)
{
    static_assert(WatchableLifetime<ScopedLifetime>);
}

TEST(ScopedLifetime, can_default_construct)
{
    [[maybe_unused]] const ScopedLifetime scoped_lifetime;
}

TEST(ScopedLifetime, can_copy_construct)
{
    const ScopedLifetime scoped_lifetime;
    [[maybe_unused]] const ScopedLifetime copy = scoped_lifetime;  // NOLINT(performance-unnecessary-copy-initialization)
}

TEST(ScopedLifetime, can_copy_assign)
{
    ScopedLifetime a;
    const ScopedLifetime b;
    a = b;
}

TEST(ScopedLifetime, watch_returns_non_expired_LifetimeWatcher)
{
    const ScopedLifetime scoped_lifetime;
    const LifetimeWatcher watcher = scoped_lifetime.watch();
    ASSERT_FALSE(watcher.expired());
}

TEST(ScopedLifetime, destructing_scoped_lifetime_causes_watcher_to_be_expired)
{
    LifetimeWatcher watcher;
    {
        const ScopedLifetime scoped_lifetime;
        watcher = scoped_lifetime.watch();
        ASSERT_FALSE(watcher.expired());
    }
    ASSERT_TRUE(watcher.expired());
}

TEST(ScopedLifetime, copying_scoped_lifetime_creates_unique_lifetime)
{
    const ScopedLifetime first_lifetime;

    LifetimeWatcher watcher;
    {
        const ScopedLifetime second_lifetime = first_lifetime;  // NOLINT(performance-unnecessary-copy-initialization)
        watcher = second_lifetime.watch();
        ASSERT_FALSE(watcher.expired());
    }
    ASSERT_TRUE(watcher.expired());
}

TEST(ScopedLifetime, copy_assignment_resets_ScopedLifetime)
{
    // the reason for this behavior is paranoia: a copy assignment might've
    // invalidated a pointer that's tied to the lifetime
    //
    // if this behavior is undesirable, then create an alternative lifetime
    // class (the lifetime API is designed to be easy-to-recompose)

    ScopedLifetime lifetime;
    const LifetimeWatcher watcher = lifetime.watch();
    ASSERT_FALSE(watcher.expired());
    const ScopedLifetime rhs;
    lifetime = rhs;
    ASSERT_TRUE(watcher.expired());
}

#include <oscar/Utils/ScopedLifetime.h>

#include <oscar/Utils/LifetimeWatcher.h>
#include <oscar/Utils/WatchableLifetime.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(ScopedLifetime, satisfied_WatchableLifetime)
{
    static_assert(WatchableLifetime<ScopedLifetime>);
}

TEST(ScopedLifetime, can_default_construct)
{
    [[maybe_unused]] ScopedLifetime scoped_lifetime;
}

TEST(ScopedLifetime, can_copy_construct)
{
    ScopedLifetime scoped_lifetime;
    [[maybe_unused]] ScopedLifetime copy = scoped_lifetime;  // NOLINT(performance-unnecessary-copy-initialization)
}

TEST(ScopedLifetime, can_copy_assign)
{
    ScopedLifetime a;
    ScopedLifetime b;
    a = b;
}

TEST(ScopedLifetime, watch_returns_non_expired_LifetimeWatcher)
{
    ScopedLifetime scoped_lifetime;
    LifetimeWatcher watcher = scoped_lifetime.watch();
    ASSERT_FALSE(watcher.expired());
}

TEST(ScopedLifetime, destructing_scoped_lifetime_causes_watcher_to_be_expired)
{
    LifetimeWatcher watcher;
    {
        ScopedLifetime scoped_lifetime;
        watcher = scoped_lifetime.watch();
        ASSERT_FALSE(watcher.expired());
    }
    ASSERT_TRUE(watcher.expired());
}

TEST(ScopedLifetime, copying_scoped_lifetime_creates_unique_lifetime)
{
    ScopedLifetime first_lifetime;

    LifetimeWatcher watcher;
    {
        ScopedLifetime second_lifetime = first_lifetime;  // NOLINT(performance-unnecessary-copy-initialization)
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
    LifetimeWatcher watcher = lifetime.watch();
    ASSERT_FALSE(watcher.expired());
    ScopedLifetime rhs;
    lifetime = rhs;
    ASSERT_TRUE(watcher.expired());
}

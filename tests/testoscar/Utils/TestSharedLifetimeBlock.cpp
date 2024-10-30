#include <oscar/Utils/SharedLifetimeBlock.h>

#include <oscar/Utils/WatchableLifetime.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(SharedLifetimeBlock, satisfies_WatchableLifetime)
{
    static_assert(WatchableLifetime<SharedLifetimeBlock>);
}

TEST(SharedLifetimeBlock, can_be_default_constructed)
{
    [[maybe_unused]] const SharedLifetimeBlock default_constructed;
}

TEST(SharedLifetimeBlock, can_be_copy_constructed)
{
    const SharedLifetimeBlock lifetime;
    [[maybe_unused]] const SharedLifetimeBlock copy = lifetime;  // NOLINT(performance-unnecessary-copy-initialization)
}

TEST(SharedLifetimeBlock, num_owners_is_initially_one)
{
    const SharedLifetimeBlock lifetime;
    ASSERT_EQ(lifetime.num_owners(), 1);
}

TEST(SharedLifetimeBlock, num_owners_increments_if_copied)
{
    const SharedLifetimeBlock lifetime;
    [[maybe_unused]] const SharedLifetimeBlock copy = lifetime;  // NOLINT(performance-unnecessary-copy-initialization)
    ASSERT_EQ(lifetime.num_owners(), 2);
}

TEST(SharedLifetimeBlock, num_owners_returns_to_1_after_copy_is_dropped)
{
    const SharedLifetimeBlock lifetime;
    {
        const SharedLifetimeBlock copy = lifetime;  // NOLINT(performance-unnecessary-copy-initialization)
        ASSERT_EQ(lifetime.num_owners(), 2);
        ASSERT_EQ(copy.num_owners(), 2);
    }
    ASSERT_EQ(lifetime.num_owners(), 1);
}

TEST(SharedLifetimeBlock, watch_returns_non_expired_LifetimeWatcher)
{
    const SharedLifetimeBlock lifetime;
    const LifetimeWatcher watcher = lifetime.watch();
    ASSERT_FALSE(watcher.expired());
}

TEST(SharedLifetimeBlock, watch_doesnt_change_num_owners)
{
    const SharedLifetimeBlock lifetime;
    ASSERT_EQ(lifetime.num_owners(), 1);
    const LifetimeWatcher watcher = lifetime.watch();
    ASSERT_EQ(lifetime.num_owners(), 1);
}

TEST(SharedLifetimeBlock, destructor_causes_watchers_to_become_expired)
{
    LifetimeWatcher watcher;
    {
        const SharedLifetimeBlock lifetime;
        watcher = lifetime.watch();
        ASSERT_FALSE(watcher.expired());
    }
    ASSERT_TRUE(watcher.expired());
}

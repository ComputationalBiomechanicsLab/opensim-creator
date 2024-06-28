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
    [[maybe_unused]] SharedLifetimeBlock default_constructed;
}

TEST(SharedLifetimeBlock, can_be_copy_constructed)
{
    SharedLifetimeBlock lifetime;
    [[maybe_unused]] SharedLifetimeBlock copy = lifetime;
}

TEST(SharedLifetimeBlock, num_owners_is_initially_one)
{
    SharedLifetimeBlock lifetime;
    ASSERT_EQ(lifetime.num_owners(), 1);
}

TEST(SharedLifetimeBlock, num_owners_increments_if_copied)
{
    SharedLifetimeBlock lifetime;
    SharedLifetimeBlock copy = lifetime;
    ASSERT_EQ(lifetime.num_owners(), 2);
}

TEST(SharedLifetimeBlock, num_owners_returns_to_1_after_copy_is_dropped)
{
    SharedLifetimeBlock lifetime;
    {
        SharedLifetimeBlock copy = lifetime;
        ASSERT_EQ(lifetime.num_owners(), 2);
        ASSERT_EQ(copy.num_owners(), 2);
    }
    ASSERT_EQ(lifetime.num_owners(), 1);
}

TEST(SharedLifetimeBlock, watch_returns_non_expired_LifetimeWatcher)
{
    SharedLifetimeBlock lifetime;
    LifetimeWatcher watcher = lifetime.watch();
    ASSERT_FALSE(watcher.expired());
}

TEST(SharedLifetimeBlock, watch_doesnt_change_num_owners)
{
    SharedLifetimeBlock lifetime;
    ASSERT_EQ(lifetime.num_owners(), 1);
    LifetimeWatcher watcher = lifetime.watch();
    ASSERT_EQ(lifetime.num_owners(), 1);
}

TEST(SharedLifetimeBlock, destructor_causes_watchers_to_become_expired)
{
    LifetimeWatcher watcher;
    {
        SharedLifetimeBlock lifetime;
        watcher = lifetime.watch();
        ASSERT_FALSE(watcher.expired());
    }
    ASSERT_TRUE(watcher.expired());
}

#include <oscar/Utils/LifetimeWatcher.h>

#include <oscar/Utils/SharedLifetimeBlock.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(LifetimeWatcher, can_default_construct)
{
    [[maybe_unused]] const LifetimeWatcher watcher;
}

TEST(LifetimeWatcher, can_construct_from_SharedLifetimeBlock)
{
    const SharedLifetimeBlock lifetime_block;
    [[maybe_unused]] const LifetimeWatcher watcher = lifetime_block.watch();
}

TEST(LifetimeWatcher, expired_returns_true_when_default_constructed)
{
    const LifetimeWatcher watcher;
    ASSERT_TRUE(watcher.expired());
}

TEST(LifetimeWatcher, expired_returns_false_when_constructed_from_SharedLifetimeBlock_that_is_alive)
{
    const SharedLifetimeBlock lifetime_block;
    const LifetimeWatcher watcher = lifetime_block.watch();
    ASSERT_FALSE(watcher.expired());
}

TEST(LifetimeWatcher, expired_becomes_true_once_SharedLifetimeBlock_is_dropped)
{
    LifetimeWatcher watcher;
    ASSERT_TRUE(watcher.expired());
    {
        const SharedLifetimeBlock lifetime_block;
        watcher = lifetime_block.watch();
        ASSERT_FALSE(watcher.expired());
    }
    ASSERT_TRUE(watcher.expired());
}

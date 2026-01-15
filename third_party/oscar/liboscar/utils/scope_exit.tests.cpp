#include "scope_exit.h"

#include <gtest/gtest.h>

using namespace osc;

TEST(ScopeExit, calls_exit_function_on_destruction)
{
    bool called = false;
    {
        ScopeExit guard{[&called]{ called = true; }};
    }
    ASSERT_TRUE(called);
}

TEST(ScopeExit, release_prevents_exit_function_from_being_called)
{
    bool called = false;
    {
        ScopeExit guard{[&called]{ called = true; }};
        guard.release();
    }
    ASSERT_FALSE(called);
}

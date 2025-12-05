#include "WindowID.h"

#include <gtest/gtest.h>

#include <bit>

using namespace osc;

TEST(WindowID, is_falsey_when_default_constructed)
{
    const WindowID id;
    ASSERT_FALSE(id);
}

TEST(WindowID, can_be_constructed_from_a_void_ptr)
{
    // this is necessary because third-party libraries (e.g. imgui) hold handles
    // to platform objects (windows, monitors, etc.) as type-erased `void*`
    [[maybe_unused]] const WindowID id{static_cast<void*>(nullptr)};
}

TEST(WindowID, is_falsey_when_constructed_from_a_null_void_ptr)
{
    ASSERT_FALSE(WindowID{static_cast<void*>(nullptr)});
}

TEST(WindowID, is_truthy_when_constructed_from_a_non_null_void_ptr)
{
    ASSERT_TRUE(WindowID{std::bit_cast<void*>(uintptr_t{0x1})});
}

TEST(WindowID, can_be_converted_to_a_void_ptr)
{
    // this is necessary because third-party libraries (e.g. imgui) hold handles
    // to platform objects (windows, monitors, etc.) as type-erased `void*`
    [[maybe_unused]] const auto* ptr = static_cast<void*>(WindowID{});
}

TEST(WindowID, converting_to_and_from_a_void_ptr_compares_equal_to_original_ID)
{
    const WindowID original_id{std::bit_cast<void*>(uintptr_t{0x1})};
    auto* const ptr_casted = static_cast<void*>(original_id);
    const WindowID reconstructed_id{ptr_casted};

    ASSERT_EQ(reconstructed_id, original_id);
}

TEST(WindowID, reset_resets_the_internal_state_to_be_falsey)
{
    WindowID id{std::bit_cast<void*>(uintptr_t{0x1})};
    ASSERT_TRUE(id);
    id.reset();
    ASSERT_FALSE(id);
}

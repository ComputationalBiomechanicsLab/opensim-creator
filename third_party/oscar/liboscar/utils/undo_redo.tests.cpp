#include "undo_redo.h"

#include <gtest/gtest.h>

using namespace osc;

TEST(UndoRedo, can_construct_for_int)
{
    const UndoRedo<int> undo_redo;
}

TEST(UndoRedo, rollback_rolls_back_to_value_initialized_head)
{
    UndoRedo<int> undo_redo;

    undo_redo.upd_scratch() = 5;

    ASSERT_EQ(undo_redo.scratch(), 5);
    undo_redo.rollback();
    ASSERT_EQ(undo_redo.scratch(), 0);
}

TEST(UndoRedo, calling_undo_when_can_undo_is_false_is_a_noop)
{
    // The implementation shouldn't throw an error or segfault if downstream
    // code calls `undo` without first checking whether `can_undo` is `true`.
    //
    // This is mostly a quality-of-life thing.

    UndoRedo<int> undo_redo;

    undo_redo.upd_scratch() = 7;

    ASSERT_EQ(undo_redo.scratch(), 7);
    ASSERT_FALSE(undo_redo.can_undo());
    undo_redo.undo();
    ASSERT_EQ(undo_redo.scratch(), 7);
    ASSERT_FALSE(undo_redo.can_redo());
}

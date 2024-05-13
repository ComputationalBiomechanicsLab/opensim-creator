#include "UndoRedo.h"

#include <oscar/Utils/Assertions.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <utility>
#include <vector>

using namespace osc;

osc::UndoRedoBase::UndoRedoBase(UndoRedoEntryBase initial_commit) :
    head_{std::move(initial_commit)}
{}
osc::UndoRedoBase::UndoRedoBase(const UndoRedoBase&) = default;
osc::UndoRedoBase::UndoRedoBase(UndoRedoBase&&) noexcept = default;
osc::UndoRedoBase& osc::UndoRedoBase::operator=(const UndoRedoBase&) = default;
osc::UndoRedoBase& osc::UndoRedoBase::operator=(UndoRedoBase&&) noexcept = default;
osc::UndoRedoBase::~UndoRedoBase() noexcept = default;

void osc::UndoRedoBase::commit_scratch(std::string_view commit_message)
{
    undo_.push_back(std::move(head_));
    head_ = impl_construct_commit_from_scratch(commit_message);
    redo_.clear();
}

const UndoRedoEntryBase& osc::UndoRedoBase::head() const
{
    return head_;
}

UID osc::UndoRedoBase::head_id() const
{
    return head_.id();
}

size_t osc::UndoRedoBase::num_undo_entries() const
{
    return undo_.size();
}

ptrdiff_t osc::UndoRedoBase::num_undo_entriesi() const
{
    return static_cast<ptrdiff_t>(num_undo_entries());
}

const osc::UndoRedoEntryBase& osc::UndoRedoBase::undo_entry_at(ptrdiff_t pos) const
{
    OSC_ASSERT_ALWAYS(pos < std::ssize(undo_));
    return undo_.rbegin()[pos];
}

void osc::UndoRedoBase::undo_to(ptrdiff_t pos)
{
    if (pos >= std::ssize(undo_)) {
        return;  // out of bounds: ignore request
    }

    const UndoRedoEntryBase old_head = head_;
    const UndoRedoEntryBase new_head = undo_.rbegin()[pos];

    // push old head onto the redo stack
    redo_.push_back(old_head);

    // copy any commits between the top of the undo stack, but shallower than
    // the requested entry, onto the redo stack
    copy(undo_.rbegin(), undo_.rbegin() + pos, std::back_inserter(redo_));
    undo_.erase((undo_.rbegin() + pos + 1).base(), undo_.end());

    head_ = new_head;
    impl_copy_assign_scratch_from_commit(new_head);
}

bool osc::UndoRedoBase::can_undo() const
{
    return not undo_.empty();
}

void osc::UndoRedoBase::undo()
{
    undo_to(0);
}

size_t osc::UndoRedoBase::num_redo_entries() const
{
    return redo_.size();
}

ptrdiff_t osc::UndoRedoBase::num_redo_entriesi() const
{
    return static_cast<ptrdiff_t>(num_redo_entries());
}

const UndoRedoEntryBase& osc::UndoRedoBase::redo_entry_at(ptrdiff_t pos) const
{
    OSC_ASSERT_ALWAYS(pos < std::ssize(redo_));
    return redo_.rbegin()[pos];
}

bool osc::UndoRedoBase::can_redo() const
{
    return not redo_.empty();
}

void osc::UndoRedoBase::redo_to(ptrdiff_t pos)
{
    if (pos >= std::ssize(redo_)) {
        return;  // out of bounds: ignore request
    }

    const UndoRedoEntryBase old_head = head_;
    const UndoRedoEntryBase new_head = redo_.rbegin()[pos];

    // push old head onto the undo stack
    undo_.push_back(old_head);

    // copy any commits between the top of the redo stack but *before* the
    // requested entry onto the redo stack
    copy(redo_.rbegin(), redo_.rbegin() + pos, std::back_inserter(undo_));
    redo_.erase((redo_.rbegin() + pos + 1).base(), redo_.end());

    head_ = new_head;
    impl_copy_assign_scratch_from_commit(new_head);
}

void osc::UndoRedoBase::redo()
{
    redo_to(0);
}

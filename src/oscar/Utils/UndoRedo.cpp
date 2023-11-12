#include "UndoRedo.hpp"

#include <oscar/Utils/Assertions.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <utility>
#include <vector>

osc::UndoRedoEntryMetadata::UndoRedoEntryMetadata(std::string_view message_) :
    m_Time{std::chrono::system_clock::now()},
    m_Message{message_}
{
}

osc::UndoRedoEntryMetadata::UndoRedoEntryMetadata(UndoRedoEntryMetadata const&) = default;
osc::UndoRedoEntryMetadata::UndoRedoEntryMetadata(UndoRedoEntryMetadata&&) noexcept = default;
osc::UndoRedoEntryMetadata& osc::UndoRedoEntryMetadata::operator=(UndoRedoEntryMetadata const&) = default;
osc::UndoRedoEntryMetadata& osc::UndoRedoEntryMetadata::operator=(UndoRedoEntryMetadata&&) noexcept = default;
osc::UndoRedoEntryMetadata::~UndoRedoEntryMetadata() noexcept = default;

osc::UndoRedo::UndoRedo(UndoRedoEntry initialCommit_) : m_Head{std::move(initialCommit_)}
{
}
osc::UndoRedo::UndoRedo(UndoRedo const&) = default;
osc::UndoRedo::UndoRedo(UndoRedo&&) noexcept = default;
osc::UndoRedo& osc::UndoRedo::operator=(UndoRedo const&) = default;
osc::UndoRedo& osc::UndoRedo::operator=(UndoRedo&&) noexcept = default;
osc::UndoRedo::~UndoRedo() noexcept = default;

void osc::UndoRedo::commitScratch(std::string_view commitMsg)
{
    m_Undo.push_back(std::move(m_Head));
    m_Head = implCreateCommitFromScratch(commitMsg);
    m_Redo.clear();
}

osc::UndoRedoEntry const& osc::UndoRedo::getHead() const
{
    return m_Head;
}

size_t osc::UndoRedo::getNumUndoEntries() const
{
    return m_Undo.size();
}

ptrdiff_t osc::UndoRedo::getNumUndoEntriesi() const
{
    return static_cast<ptrdiff_t>(getNumUndoEntries());
}

osc::UndoRedoEntry const& osc::UndoRedo::getUndoEntry(ptrdiff_t i) const
{
    OSC_ASSERT(i < std::ssize(m_Undo));
    return m_Undo.rbegin()[i];
}

void osc::UndoRedo::undoTo(ptrdiff_t nthEntry)
{
    if (nthEntry >= std::ssize(m_Undo))
    {
        return;  // out of bounds: ignore request
    }

    UndoRedoEntry const oldHead = m_Head;
    UndoRedoEntry const newHead = m_Undo.rbegin()[nthEntry];

    // push old head onto the redo stack
    m_Redo.push_back(oldHead);

    // copy any commits between the top of the undo stack, but shallower than
    // the requested entry, onto the redo stack
    std::copy(m_Undo.rbegin(), m_Undo.rbegin() + nthEntry, std::back_inserter(m_Redo));
    m_Undo.erase((m_Undo.rbegin() + nthEntry + 1).base(), m_Undo.end());

    m_Head = newHead;
    implAssignScratchFromCommit(newHead);
}

bool osc::UndoRedo::canUndo() const
{
    return !m_Undo.empty();
}

void osc::UndoRedo::undo()
{
    undoTo(0);
}

size_t osc::UndoRedo::getNumRedoEntries() const
{
    return m_Redo.size();
}

ptrdiff_t osc::UndoRedo::getNumRedoEntriesi() const
{
    return static_cast<ptrdiff_t>(getNumRedoEntries());
}

osc::UndoRedoEntry const& osc::UndoRedo::getRedoEntry(ptrdiff_t i) const
{
    OSC_ASSERT(i < std::ssize(m_Redo));
    return m_Redo.rbegin()[i];
}

bool osc::UndoRedo::canRedo() const
{
    return !m_Redo.empty();
}

void osc::UndoRedo::redoTo(ptrdiff_t nthEntry)
{
    if (nthEntry >= std::ssize(m_Redo))
    {
        return;  // out of bounds: ignore request
    }

    UndoRedoEntry const oldHead = m_Head;
    UndoRedoEntry const newHead = m_Redo.rbegin()[nthEntry];

    // push old head onto the undo stack
    m_Undo.push_back(oldHead);

    // copy any commits between the top of the redo stack but *before* the
    // requested entry onto the redo stack
    std::copy(m_Redo.rbegin(), m_Redo.rbegin() + nthEntry, std::back_inserter(m_Undo));
    m_Redo.erase((m_Redo.rbegin() + nthEntry + 1).base(), m_Redo.end());

    m_Head = newHead;
    implAssignScratchFromCommit(newHead);
}

void osc::UndoRedo::redo()
{
    redoTo(0);
}

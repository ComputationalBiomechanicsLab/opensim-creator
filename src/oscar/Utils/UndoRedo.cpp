#include "UndoRedo.h"

#include <oscar/Utils/Assertions.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <utility>
#include <vector>

using namespace osc;

osc::UndoRedoBase::UndoRedoBase(UndoRedoEntryBase initialCommit_) : m_Head{std::move(initialCommit_)}
{
}
osc::UndoRedoBase::UndoRedoBase(UndoRedoBase const&) = default;
osc::UndoRedoBase::UndoRedoBase(UndoRedoBase&&) noexcept = default;
osc::UndoRedoBase& osc::UndoRedoBase::operator=(UndoRedoBase const&) = default;
osc::UndoRedoBase& osc::UndoRedoBase::operator=(UndoRedoBase&&) noexcept = default;
osc::UndoRedoBase::~UndoRedoBase() noexcept = default;

void osc::UndoRedoBase::commitScratch(std::string_view commitMsg)
{
    m_Undo.push_back(std::move(m_Head));
    m_Head = implCreateCommitFromScratch(commitMsg);
    m_Redo.clear();
}

UndoRedoEntryBase const& osc::UndoRedoBase::getHead() const
{
    return m_Head;
}

UID osc::UndoRedoBase::getHeadID() const
{
    return m_Head.id();
}

size_t osc::UndoRedoBase::getNumUndoEntries() const
{
    return m_Undo.size();
}

ptrdiff_t osc::UndoRedoBase::getNumUndoEntriesi() const
{
    return static_cast<ptrdiff_t>(getNumUndoEntries());
}

osc::UndoRedoEntryBase const& osc::UndoRedoBase::getUndoEntry(ptrdiff_t i) const
{
    OSC_ASSERT(i < std::ssize(m_Undo));
    return m_Undo.rbegin()[i];
}

void osc::UndoRedoBase::undoTo(ptrdiff_t nthEntry)
{
    if (nthEntry >= std::ssize(m_Undo))
    {
        return;  // out of bounds: ignore request
    }

    UndoRedoEntryBase const oldHead = m_Head;
    UndoRedoEntryBase const newHead = m_Undo.rbegin()[nthEntry];

    // push old head onto the redo stack
    m_Redo.push_back(oldHead);

    // copy any commits between the top of the undo stack, but shallower than
    // the requested entry, onto the redo stack
    copy(m_Undo.rbegin(), m_Undo.rbegin() + nthEntry, std::back_inserter(m_Redo));
    m_Undo.erase((m_Undo.rbegin() + nthEntry + 1).base(), m_Undo.end());

    m_Head = newHead;
    implAssignScratchFromCommit(newHead);
}

bool osc::UndoRedoBase::canUndo() const
{
    return !m_Undo.empty();
}

void osc::UndoRedoBase::undo()
{
    undoTo(0);
}

size_t osc::UndoRedoBase::getNumRedoEntries() const
{
    return m_Redo.size();
}

ptrdiff_t osc::UndoRedoBase::getNumRedoEntriesi() const
{
    return static_cast<ptrdiff_t>(getNumRedoEntries());
}

UndoRedoEntryBase const& osc::UndoRedoBase::getRedoEntry(ptrdiff_t i) const
{
    OSC_ASSERT(i < std::ssize(m_Redo));
    return m_Redo.rbegin()[i];
}

bool osc::UndoRedoBase::canRedo() const
{
    return !m_Redo.empty();
}

void osc::UndoRedoBase::redoTo(ptrdiff_t nthEntry)
{
    if (nthEntry >= std::ssize(m_Redo))
    {
        return;  // out of bounds: ignore request
    }

    UndoRedoEntryBase const oldHead = m_Head;
    UndoRedoEntryBase const newHead = m_Redo.rbegin()[nthEntry];

    // push old head onto the undo stack
    m_Undo.push_back(oldHead);

    // copy any commits between the top of the redo stack but *before* the
    // requested entry onto the redo stack
    copy(m_Redo.rbegin(), m_Redo.rbegin() + nthEntry, std::back_inserter(m_Undo));
    m_Redo.erase((m_Redo.rbegin() + nthEntry + 1).base(), m_Redo.end());

    m_Head = newHead;
    implAssignScratchFromCommit(newHead);
}

void osc::UndoRedoBase::redo()
{
    redoTo(0);
}

#include "UndoableUiModel.hpp"

#include "src/Assertions.hpp"

#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/UiModel.hpp"
#include "src/Utils/CircularBuffer.hpp"
#include "src/Utils/UID.hpp"
#include "src/Log.hpp"

#include <OpenSim/Simulation/Model/Model.h>

#include <chrono>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <utility>

using namespace osc;
using std::chrono_literals::operator""s;

// commit support
//
// a datastructure that holds commit-level information about a UiModel
namespace
{
    // a single "commit" of the model graph for undo/redo storage
    class UiModelCommit final {
    public:
        using Clock = std::chrono::system_clock;

        explicit UiModelCommit(UiModel model) :
            m_Model{std::move(model)}
        {
        }

        UiModelCommit(UiModel model, UID parent) :
            m_MaybeParentID{std::move(parent)},
            m_Model{std::move(model)}
        {
        }

        UID getID() const { return m_ID; }
        bool hasParent() const { return m_MaybeParentID != UID::empty(); }
        UID getParentID() const { return m_MaybeParentID; }
        Clock::time_point getCommitTime() const { return m_CommitTime; }
        UiModel const& getUiModel() const { return m_Model; }

    private:
        // unique ID for this commit
        UID m_ID;

        // (maybe) unique ID of the parent commit
        UID m_MaybeParentID = UID::empty();

        // when the commit was created
        Clock::time_point m_CommitTime = Clock::now();

        // the model saved in this snapshot
        UiModel m_Model;
    };
}

class osc::UndoableUiModel::Impl final {
public:

    Impl()
    {
        m_Scratch.updateIfDirty();
        commit();  // make initial commit
    }

    // crete a new commit graph that contains a backup of the given model
    explicit Impl(std::unique_ptr<OpenSim::Model> m) :
        m_Scratch{std::move(m)},
        m_MaybeFilesystemLocation{TryFindInputFile(m_Scratch.getModel())}
    {
        m_Scratch.updateIfDirty();
        commit();  // make initial commit
    }

    // commit scratch to storage
    UID commit()
    {
        auto commit = UiModelCommit{m_Scratch, m_CurrentHead};
        UID commitID = commit.getID();

        m_Commits.try_emplace(commitID, std::move(commit));
        m_CurrentHead = commitID;
        m_BranchHead = commitID;

        return commitID;
    }

    // try to lookup a commit by its ID
    UiModelCommit const* tryGetCommitByID(UID id) const
    {
        auto it = m_Commits.find(id);
        return it != m_Commits.end() ? &it->second : nullptr;
    }

    UiModelCommit const& getHeadCommit() const
    {
        OSC_ASSERT(m_CurrentHead != UID::empty());
        OSC_ASSERT(hasCommit(m_CurrentHead));

        return *tryGetCommitByID(m_CurrentHead);
    }

    // try to lookup the *parent* of a given commit, or return an empty (senteniel) ID
    UID tryGetParentIDOrEmpty(UID id) const
    {
        UiModelCommit const* commit = tryGetCommitByID(id);
        return commit ? commit->getParentID() : UID::empty();
    }

    // returns `true` if a commit with the given ID has been stored
    bool hasCommit(UID id) const
    {
        return tryGetCommitByID(id);
    }

    // returns the number of hops between commit `a` and commit `b`
    //
    // returns -1 if commit `b` cannot be reached from commit `a`
    int distance(UID a, UID b) const
    {
        if (a == b)
        {
            return 0;
        }

        int n = 1;
        UID parent = tryGetParentIDOrEmpty(a);

        while (parent != b && parent != UID::empty())
        {
            parent = tryGetParentIDOrEmpty(parent);
            n++;
        }

        return parent == b ? n : -1;
    }

    // returns a pointer to a commit that is the nth ancestor from `a`
    //
    // (e.g. n==0 returns `a`, n==1 returns `a`'s parent, n==2 returns `a`'s grandparent)
    //
    // returns `nullptr` if there are insufficient ancestors. `n` must be >= 0
    UiModelCommit const* nthAncestor(UID a, int n) const
    {
        if (n < 0)
        {
            return nullptr;
        }

        UiModelCommit const* c = tryGetCommitByID(a);

        if (!c || n == 0)
        {
            return c;
        }

        int i = 1;
        c = tryGetCommitByID(c->getParentID());

        while (c && i < n)
        {
            c = tryGetCommitByID(c->getParentID());
            i++;
        }

        return c;
    }

    // returns the UID that is the nth ancestor from `a`, or empty if there are insufficient ancestors
    UID nthAncestorID(UID a, int n) const
    {
        UiModelCommit const* c = nthAncestor(a, n);
        return c ? c->getID() : UID::empty();
    }

    // returns `true` if `maybeAncestor` is an ancestor of `id`
    bool isAncestor(UID maybeAncestor, UID id)
    {
        UiModelCommit const* c = tryGetCommitByID(id);

        while (c && c->getID() != maybeAncestor)
        {
            c = tryGetCommitByID(c->getParentID());
        }

        return c;
    }

    // remove a range of commits from `start` (inclusive) to `end` (exclusive)
    void eraseCommitRange(UID start, UID end)
    {
        auto it = m_Commits.find(start);

        while (it != m_Commits.end() && it->second.getID() != end)
        {
            UID parent = it->second.getParentID();
            m_Commits.erase(it);

            it = m_Commits.find(parent);
        }
    }

    // garbage collect (erase) commits that fall outside the maximum undo depth
    void garbageCollectMaxUndo()
    {
        static_assert(m_MaxUndo >= 0);

        UID firstBadCommitIDOrEmpty = nthAncestorID(m_CurrentHead, m_MaxUndo + 1);
        eraseCommitRange(firstBadCommitIDOrEmpty, UID::empty());
    }

    // garbage collect (erase) commits that fall outside the maximum redo depth
    void garbageCollectMaxRedo()
    {
        static_assert(m_MaxRedo >= 0);

        int numRedos = distance(m_BranchHead, m_CurrentHead);
        int numDeletions = numRedos - m_MaxRedo;

        if (numDeletions <= 0)
        {
            return;
        }

        UID newBranchHead = nthAncestorID(m_BranchHead, numDeletions);
        eraseCommitRange(m_BranchHead, newBranchHead);
        m_BranchHead = newBranchHead;
    }

    void garbageCollectUnreachable()
    {
        for (auto it = m_Commits.begin(); it != m_Commits.end();)
        {
            if (it->first == m_BranchHead || isAncestor(it->first, m_BranchHead))
            {
                ++it;
            }
            else
            {
                it = m_Commits.erase(it);
            }
        }
    }

    // remove out-of-bounds, deleted, out-of-date, etc. commits
    void garbageCollect()
    {
        garbageCollectMaxUndo();
        garbageCollectMaxRedo();
        garbageCollectUnreachable();
    }

    // returns commit ID of the currently active checkout
    UID getCheckoutID() const
    {
        return m_CurrentHead;
    }

    // checks out the current checkout to be active (scratch)
    //
    // effectively, reset the scratch space
    void checkout()
    {
        // because this is a "reset", try to maintain useful state from the
        // scratch space - things like reset and scaling state, which the
        // user might expect to be maintained even if a crash happened

        UiModelCommit const* c = tryGetCommitByID(m_CurrentHead);

        if (c)
        {
            UiModel newScratch = c->getUiModel();
            newScratch.setSelectedHoveredAndIsolatedFrom(m_Scratch);
            newScratch.setFixupScaleFactor(m_Scratch.getFixupScaleFactor());
            newScratch.updateIfDirty();

            m_Scratch = std::move(newScratch);
        }
    }

    // returns true if the an undo is possible
    bool canUndo() const
    {
        UiModelCommit const* c = tryGetCommitByID(m_CurrentHead);
        return c ? hasCommit(c->getParentID()) : false;
    }

    // performs an undo, if possible
    //
    // effectively, checks out HEAD~1
    void undo()
    {
        UiModelCommit const* c = tryGetCommitByID(m_CurrentHead);

        if (!c)
        {
            return;
        }

        UiModelCommit const* parent = tryGetCommitByID(c->getParentID());

        if (!parent)
        {
            return;
        }

        // perform hacky fixups to ensure the user experience is best:
        //
        // - user's selection state should be "sticky" between undo/redo
        // - user's scene scale factor should be "sticky" between undo/redo
        UiModel newModel = parent->getUiModel();
        newModel.setSelectedHoveredAndIsolatedFrom(m_Scratch);
        newModel.setFixupScaleFactor(m_Scratch.getFixupScaleFactor());
        newModel.updateIfDirty();

        OSC_ASSERT(newModel.getModelVersion() == parent->getUiModel().getModelVersion());
        OSC_ASSERT(newModel.getStateVersion() == parent->getUiModel().getStateVersion());

        m_Scratch = std::move(newModel);
        m_CurrentHead = parent->getID();

        OSC_ASSERT(m_Scratch.getModelVersion() == getHeadCommit().getUiModel().getModelVersion());
        OSC_ASSERT(m_Scratch.getStateVersion() == getHeadCommit().getUiModel().getStateVersion());
    }

    // returns true if a redo is possible
    bool canRedo() const
    {
        return distance(m_BranchHead, m_CurrentHead) > 0;
    }

    // performs a redo, if possible
    void redo()
    {
        int dist = distance(m_BranchHead, m_CurrentHead);

        if (dist < 1)
        {
            return;
        }

        UiModelCommit const* c = nthAncestor(m_BranchHead, dist - 1);

        if (!c)
        {
            return;
        }

        // perform hacky fixups to ensure the user experience is best:
        //
        // - user's selection state should be "sticky" between undo/redo
        // - user's scene scale factor should be "sticky" between undo/redo
        UiModel newModel = c->getUiModel();
        newModel.setSelectedHoveredAndIsolatedFrom(m_Scratch);
        newModel.setFixupScaleFactor(m_Scratch.getFixupScaleFactor());
        newModel.updateIfDirty();

        m_Scratch = std::move(newModel);
        m_CurrentHead = c->getID();
    }

    UiModel& updScratch()
    {
        return m_Scratch;
    }

    UiModel const& getScratch()
    {
        return m_Scratch;
    }

    std::filesystem::path const& getFilesystemLocation() const
    {
        return m_MaybeFilesystemLocation;
    }

    void setFilesystemLocation(std::filesystem::path const& p)
    {
        m_MaybeFilesystemLocation = p;
    }

    UID getFilesystemVersion() const
    {
        return m_MaybeCommitSavedToDisk;
    }

    void setFilesystemVersionToCurrent()
    {
        m_MaybeCommitSavedToDisk = m_CurrentHead;
    }

private:
    // mutable staging area that calling code can mutate
    UiModel m_Scratch;

    // where scratch will commit to (i.e. the parent of the scratch area)
    UID m_CurrentHead = UID::empty();

    // head of the current branch (i.e. "master") - may be ahead of current branch (undo/redo)
    UID m_BranchHead = UID::empty();

    // maximum distance between the current commit and the "root" commit (i.e. a commit with no parent)
    static constexpr int m_MaxUndo = 32;

    // maximum distance between the branch head and the current commit (i.e. how big the redo buffer can be)
    static constexpr int m_MaxRedo = 32;

    // underlying storage for immutable commits
    std::unordered_map<UID, UiModelCommit> m_Commits;

    // (maybe) the location of the model on-disk
    std::filesystem::path m_MaybeFilesystemLocation;

    // (maybe) the version of the model that was last saved to disk
    UID m_MaybeCommitSavedToDisk = UID::empty();
};


// public API

osc::UndoableUiModel::UndoableUiModel() :
    m_Impl{new Impl{}}
{
}

osc::UndoableUiModel::UndoableUiModel(std::unique_ptr<OpenSim::Model> model) :
    m_Impl{new Impl{std::move(model)}}
{
}

osc::UndoableUiModel::UndoableUiModel(UndoableUiModel const& src) :
    m_Impl{new Impl{*src.m_Impl}}
{
}

osc::UndoableUiModel::UndoableUiModel(UndoableUiModel&&) noexcept = default;

osc::UndoableUiModel::~UndoableUiModel() noexcept = default;

UndoableUiModel& osc::UndoableUiModel::operator=(UndoableUiModel const& src)
{
    if (&src != this)
    {
        std::unique_ptr<Impl> cpy = std::make_unique<Impl>(*src.m_Impl);
        std::swap(m_Impl, cpy);
    }

    return *this;
}

UndoableUiModel& osc::UndoableUiModel::operator=(UndoableUiModel&&) noexcept = default;

bool osc::UndoableUiModel::hasFilesystemLocation() const
{
    return !m_Impl->getFilesystemLocation().empty();
}

std::filesystem::path const& osc::UndoableUiModel::getFilesystemPath() const
{
    return m_Impl->getFilesystemLocation();
}

void osc::UndoableUiModel::setFilesystemPath(std::filesystem::path const& p)
{
    m_Impl->setFilesystemLocation(p);
}

bool osc::UndoableUiModel::isUpToDateWithFilesystem() const
{
    return m_Impl->getCheckoutID() == m_Impl->getFilesystemVersion();
}

void osc::UndoableUiModel::setUpToDateWithFilesystem()
{
    m_Impl->setFilesystemVersionToCurrent();
}

UiModel const& osc::UndoableUiModel::getUiModel() const
{
    return m_Impl->getScratch();
}

UiModel& osc::UndoableUiModel::updUiModel()
{
    return m_Impl->updScratch();
}

bool osc::UndoableUiModel::canUndo() const
{
    return m_Impl->canUndo();
}

void osc::UndoableUiModel::doUndo()
{
    if (!m_Impl->canUndo())
    {
        return;
    }

    m_Impl->undo();
}

void osc::UndoableUiModel::rollback()
{
    m_Impl->checkout();
}

bool osc::UndoableUiModel::canRedo() const
{
    return m_Impl->canRedo();
}

void osc::UndoableUiModel::doRedo()
{
    if (!canRedo())
    {
        return;
    }

    m_Impl->redo();
}

OpenSim::Model const& osc::UndoableUiModel::getModel() const
{
    return m_Impl->getScratch().getModel();
}

OpenSim::Model& osc::UndoableUiModel::updModel()
{
    return m_Impl->updScratch().updModel();
}

void osc::UndoableUiModel::setModel(std::unique_ptr<OpenSim::Model> newModel)
{
    updUiModel().setModel(std::move(newModel));
    updateIfDirty();
}

SimTK::State const& osc::UndoableUiModel::getState() const
{
    return getUiModel().getState();
}

float osc::UndoableUiModel::getFixupScaleFactor() const
{
    return getUiModel().getFixupScaleFactor();
}

void osc::UndoableUiModel::setFixupScaleFactor(float v)
{
    updUiModel().setFixupScaleFactor(v);
}

float osc::UndoableUiModel::getReccommendedScaleFactor() const
{
    return getUiModel().getRecommendedScaleFactor();
}

void osc::UndoableUiModel::updateIfDirty()
{
    UiModel& scratch = m_Impl->updScratch();

    // ensure the scratch space is clean
    try
    {
        scratch.updateIfDirty();
    }
    catch (std::exception const& ex)
    {
        log::error("exception occurred after applying changes to a model:");
        log::error("%s", ex.what());
        log::error("attempting to rollback to an earlier version of the model");
        m_Impl->checkout();
    }

    // HACK: auto-perform commit (if necessary)
    {
        UiModelCommit const& head = m_Impl->getHeadCommit();
        auto debounceTime = 2s;

        bool scratchContainsModelChanges = scratch.getModelVersion() != head.getUiModel().getModelVersion();
        bool scratchContainsStateChanges = scratch.getStateVersion() != head.getUiModel().getStateVersion();
        bool lastCommitWasAWhileAgo = head.getCommitTime() < (UiModelCommit::Clock::now() - debounceTime);

        if ((scratchContainsModelChanges || scratchContainsStateChanges) && lastCommitWasAWhileAgo)
        {
            log::debug("commit model to undo/redo storage");
            m_Impl->commit();
        }
    }
}

void osc::UndoableUiModel::setDirty(bool v)
{
    updUiModel().setDirty(v);
}

bool osc::UndoableUiModel::hasSelected() const
{
    return getUiModel().hasSelected();
}

OpenSim::Component const* osc::UndoableUiModel::getSelected() const
{
    return getUiModel().getSelected();
}

OpenSim::Component* osc::UndoableUiModel::updSelected()
{
    return updUiModel().updSelected();
}

void osc::UndoableUiModel::setSelected(OpenSim::Component const* c)
{
    updUiModel().setSelected(c);
}

bool osc::UndoableUiModel::selectionHasTypeHashCode(size_t v) const
{
    return getUiModel().selectionHasTypeHashCode(v);
}

bool osc::UndoableUiModel::hasHovered() const
{
    return getUiModel().hasHovered();
}

OpenSim::Component const* osc::UndoableUiModel::getHovered() const
{
    return getUiModel().getHovered();
}

OpenSim::Component* osc::UndoableUiModel::updHovered()
{
    return updUiModel().updHovered();
}

void osc::UndoableUiModel::setHovered(OpenSim::Component const* c)
{
    updUiModel().setHovered(c);
}

OpenSim::Component const* osc::UndoableUiModel::getIsolated() const
{
    return getUiModel().getIsolated();
}

OpenSim::Component* osc::UndoableUiModel::updIsolated()
{
    return updUiModel().updIsolated();
}

void osc::UndoableUiModel::setIsolated(OpenSim::Component const* c)
{
    updUiModel().setIsolated(c);
}

void osc::UndoableUiModel::declareDeathOf(const OpenSim::Component *c)
{
    updUiModel().declareDeathOf(c);
}

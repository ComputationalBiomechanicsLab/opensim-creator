#include "UndoableModelStatePair.hpp"

#include "src/OpenSimBindings/AutoFinalizingModelStatePair.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/Platform/Log.hpp"
#include "src/Utils/Assertions.hpp"
#include "src/Utils/Perf.hpp"
#include "src/Utils/UID.hpp"

#include <OpenSim/Simulation/Model/Model.h>

#include <chrono>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <utility>

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

        explicit UiModelCommit(osc::AutoFinalizingModelStatePair model, std::string_view message) :
            m_Model{std::move(model)},
            m_Message{std::move(message)}
        {
        }

        UiModelCommit(osc::AutoFinalizingModelStatePair model, osc::UID parent, std::string_view message) :
            m_MaybeParentID{std::move(parent)},
            m_Model{std::move(model)},
            m_Message{std::move(message)}
        {
        }

        osc::UID getID() const { return m_ID; }
        bool hasParent() const { return m_MaybeParentID != osc::UID::empty(); }
        osc::UID getParentID() const { return m_MaybeParentID; }
        Clock::time_point getCommitTime() const { return m_CommitTime; }
        osc::AutoFinalizingModelStatePair const& getUiModel() const { return m_Model; }

    private:
        // unique ID for this commit
        osc::UID m_ID;

        // (maybe) unique ID of the parent commit
        osc::UID m_MaybeParentID = osc::UID::empty();

        // when the commit was created
        Clock::time_point m_CommitTime = Clock::now();

        // the model saved in this snapshot
        osc::AutoFinalizingModelStatePair m_Model;

        // commit message
        std::string m_Message;
    };
}

class osc::UndoableModelStatePair::Impl final {
public:

    Impl()
    {
        m_Scratch.updateIfDirty();
        doCommit("initial commit");  // make initial commit
    }

    // crete a new commit graph that contains a backup of the given model
    explicit Impl(std::unique_ptr<OpenSim::Model> m) :
        m_Scratch{std::move(m)},
        m_MaybeFilesystemLocation{TryFindInputFile(m_Scratch.getModel())}
    {
        m_Scratch.updateIfDirty();
        doCommit("initial commit");  // make initial commit
    }

    bool hasFilesystemLocation() const
    {
        return !getFilesystemLocation().empty();
    }

    std::filesystem::path const& getFilesystemPath() const
    {
        return getFilesystemLocation();
    }

    void setFilesystemPath(std::filesystem::path const& p)
    {
        setFilesystemLocation(p);
    }

    bool isUpToDateWithFilesystem() const
    {
        return getCheckoutID() == getFilesystemVersion();
    }

    void setUpToDateWithFilesystem()
    {
        setFilesystemVersionToCurrent();
    }

    AutoFinalizingModelStatePair const& getUiModel() const
    {
        return getScratch();
    }

    AutoFinalizingModelStatePair& updUiModel()
    {
        return updScratch();
    }

    bool canUndo() const
    {
        UiModelCommit const* c = tryGetCommitByID(m_CurrentHead);
        return c ? hasCommit(c->getParentID()) : false;
    }

    void doUndo()
    {
        if (canUndo())
        {
            undo();
        }
    }

    bool canRedo() const
    {
        return distance(m_BranchHead, m_CurrentHead) > 0;
    }

    void doRedo()
    {
        if (canRedo())
        {
            redo();
        }        
    }

    void commit(std::string_view message)
    {
        AutoFinalizingModelStatePair& scratch = updScratch();

        // ensure the scratch space is clean
        try
        {
            OSC_PERF("commit model");
            scratch.updateIfDirty();
            doCommit(std::move(message));
        }
        catch (std::exception const& ex)
        {
            log::error("exception occurred after applying changes to a model:");
            log::error("    %s", ex.what());
            log::error("attempting to rollback to an earlier version of the model");
            rollback();
        }
    }

    void rollback()
    {
        checkout(true);  // care: skip copying selection because a rollback is aggro
    }

    OpenSim::Model const& getModel() const
    {
        return getScratch().getModel();
    }

    OpenSim::Model& updModel()
    {
        return updScratch().updModel();
    }

    void setModel(std::unique_ptr<OpenSim::Model> newModel)
    {
        updUiModel().setModel(std::move(newModel));
    }

    UID getModelVersion() const
    {
        return getUiModel().getModelVersion();
    }

    SimTK::State const& getState() const
    {
        return getUiModel().getState();
    }

    UID getStateVersion() const
    {
        return getUiModel().getStateVersion();
    }

    float getFixupScaleFactor() const
    {
        return getUiModel().getFixupScaleFactor();
    }

    void setFixupScaleFactor(float v)
    {
        updUiModel().setFixupScaleFactor(v);
    }

    void setDirty(bool v)
    {
        updUiModel().setDirty(v);
    }

    OpenSim::Component const* getSelected() const
    {
        return getUiModel().getSelected();
    }

    OpenSim::Component* updSelected()
    {
        return updUiModel().updSelected();
    }

    void setSelected(OpenSim::Component const* c)
    {
        updUiModel().setSelected(c);
    }

    OpenSim::Component const* getHovered() const
    {
        return getUiModel().getHovered();
    }

    OpenSim::Component* updHovered()
    {
        return updUiModel().updHovered();
    }

    void setHovered(OpenSim::Component const* c)
    {
        updUiModel().setHovered(c);
    }

    OpenSim::Component const* getIsolated() const
    {
        return getUiModel().getIsolated();
    }

    OpenSim::Component* updIsolated()
    {
        return updUiModel().updIsolated();
    }

    void setIsolated(OpenSim::Component const* c)
    {
        updUiModel().setIsolated(c);
    }

private:

    UID doCommit(std::string_view message)
    {
        auto commit = UiModelCommit{m_Scratch, m_CurrentHead, std::move(message)};
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
    void checkout(bool skipCopyingSelection = false)
    {
        // because this is a "reset", try to maintain useful state from the
        // scratch space - things like reset and scaling state, which the
        // user might expect to be maintained even if a crash happened

        UiModelCommit const* c = tryGetCommitByID(m_CurrentHead);

        if (c)
        {
            AutoFinalizingModelStatePair newScratch = c->getUiModel();
            if (!skipCopyingSelection)
            {
                // care: skipping this copy can be necessary because getSelected etc. might rethrow
                newScratch.setSelectedHoveredAndIsolatedFrom(m_Scratch);
            }
            newScratch.setFixupScaleFactor(m_Scratch.getFixupScaleFactor());
            newScratch.updateIfDirty();

            m_Scratch = std::move(newScratch);
        }
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
        AutoFinalizingModelStatePair newModel = parent->getUiModel();
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
        AutoFinalizingModelStatePair newModel = c->getUiModel();
        newModel.setSelectedHoveredAndIsolatedFrom(m_Scratch);
        newModel.setFixupScaleFactor(m_Scratch.getFixupScaleFactor());
        newModel.updateIfDirty();

        m_Scratch = std::move(newModel);
        m_CurrentHead = c->getID();
    }

    AutoFinalizingModelStatePair& updScratch()
    {
        return m_Scratch;
    }

    AutoFinalizingModelStatePair const& getScratch() const
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
    AutoFinalizingModelStatePair m_Scratch;

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


// public API (PIMPL)

osc::UndoableModelStatePair::UndoableModelStatePair() :
    m_Impl{new Impl{}}
{
}

osc::UndoableModelStatePair::UndoableModelStatePair(std::unique_ptr<OpenSim::Model> model) :
    m_Impl{new Impl{std::move(model)}}
{
}

osc::UndoableModelStatePair::UndoableModelStatePair(UndoableModelStatePair const& src) :
    m_Impl{new Impl{*src.m_Impl}}
{
}

osc::UndoableModelStatePair::UndoableModelStatePair(UndoableModelStatePair&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::UndoableModelStatePair& osc::UndoableModelStatePair::operator=(UndoableModelStatePair const& src)
{
    if (&src != this)
    {
        std::unique_ptr<Impl> cpy = std::make_unique<Impl>(*src.m_Impl);
        cpy.reset(std::exchange(m_Impl, cpy.release()));
    }

    return *this;
}

osc::UndoableModelStatePair& osc::UndoableModelStatePair::operator=(UndoableModelStatePair&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::UndoableModelStatePair::~UndoableModelStatePair() noexcept
{
    delete m_Impl;
}

bool osc::UndoableModelStatePair::hasFilesystemLocation() const
{
    return m_Impl->hasFilesystemLocation();
}

std::filesystem::path const& osc::UndoableModelStatePair::getFilesystemPath() const
{
    return m_Impl->getFilesystemPath();
}

void osc::UndoableModelStatePair::setFilesystemPath(std::filesystem::path const& p)
{
    m_Impl->setFilesystemPath(p);
}

bool osc::UndoableModelStatePair::isUpToDateWithFilesystem() const
{
    return m_Impl->isUpToDateWithFilesystem();
}

void osc::UndoableModelStatePair::setUpToDateWithFilesystem()
{
    m_Impl->setUpToDateWithFilesystem();
}

osc::AutoFinalizingModelStatePair const& osc::UndoableModelStatePair::getUiModel() const
{
    return m_Impl->getUiModel();
}

osc::AutoFinalizingModelStatePair& osc::UndoableModelStatePair::updUiModel()
{
    return m_Impl->updUiModel();
}

bool osc::UndoableModelStatePair::canUndo() const
{
    return m_Impl->canUndo();
}

void osc::UndoableModelStatePair::doUndo()
{
    m_Impl->doUndo();
}

bool osc::UndoableModelStatePair::canRedo() const
{
    return m_Impl->canRedo();
}

void osc::UndoableModelStatePair::doRedo()
{
    m_Impl->doRedo();
}

void osc::UndoableModelStatePair::commit(std::string_view message)
{
    m_Impl->commit(std::move(message));
}

void osc::UndoableModelStatePair::rollback()
{
    m_Impl->rollback();
}

OpenSim::Model const& osc::UndoableModelStatePair::getModel() const
{
    return m_Impl->getModel();
}

OpenSim::Model& osc::UndoableModelStatePair::updModel()
{
    return m_Impl->updModel();
}

void osc::UndoableModelStatePair::setModel(std::unique_ptr<OpenSim::Model> newModel)
{
    m_Impl->setModel(std::move(newModel));
}

osc::UID osc::UndoableModelStatePair::getModelVersion() const
{
    return m_Impl->getModelVersion();
}

SimTK::State const& osc::UndoableModelStatePair::getState() const
{
    return m_Impl->getState();
}

osc::UID osc::UndoableModelStatePair::getStateVersion() const
{
    return m_Impl->getStateVersion();
}

float osc::UndoableModelStatePair::getFixupScaleFactor() const
{
    return m_Impl->getFixupScaleFactor();
}

void osc::UndoableModelStatePair::setFixupScaleFactor(float v)
{
    m_Impl->setFixupScaleFactor(std::move(v));
}

void osc::UndoableModelStatePair::setDirty(bool v)
{
    m_Impl->setDirty(std::move(v));
}

OpenSim::Component const* osc::UndoableModelStatePair::getSelected() const
{
    return m_Impl->getSelected();
}

OpenSim::Component* osc::UndoableModelStatePair::updSelected()
{
    return m_Impl->updSelected();
}

void osc::UndoableModelStatePair::setSelected(OpenSim::Component const* c)
{
    m_Impl->setSelected(std::move(c));
}

OpenSim::Component const* osc::UndoableModelStatePair::getHovered() const
{
    return m_Impl->getHovered();
}

OpenSim::Component* osc::UndoableModelStatePair::updHovered()
{
    return m_Impl->updHovered();
}

void osc::UndoableModelStatePair::setHovered(OpenSim::Component const* c)
{
    m_Impl->setHovered(std::move(c));
}

OpenSim::Component const* osc::UndoableModelStatePair::getIsolated() const
{
    return m_Impl->getIsolated();
}

OpenSim::Component* osc::UndoableModelStatePair::updIsolated()
{
    return m_Impl->updIsolated();
}

void osc::UndoableModelStatePair::setIsolated(OpenSim::Component const* c)
{
    m_Impl->setIsolated(std::move(c));
}

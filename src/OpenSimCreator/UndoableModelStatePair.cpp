#include "UndoableModelStatePair.hpp"

#include "OpenSimCreator/ModelStateCommit.hpp"
#include "OpenSimCreator/OpenSimHelpers.hpp"

#include <oscar/Platform/Log.hpp>
#include <oscar/Utils/Assertions.hpp>
#include <oscar/Utils/Perf.hpp>
#include <oscar/Utils/SynchronizedValue.hpp>
#include <oscar/Utils/UID.hpp>

#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Common/ModelDisplayHints.h>
#include <OpenSim/Common/PropertyObjArray.h>
#include <OpenSim/Common/Set.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <exception>
#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// maximum distance between the current commit and the "root" commit (i.e. a commit with no parent)
static inline int constexpr c_MaxUndo = 32;

// maximum distance between the branch head and the current commit (i.e. how big the redo buffer can be)
static inline int constexpr c_MaxRedo = 32;

namespace
{
    std::unique_ptr<OpenSim::Model> makeNewModel()
    {
        auto rv = std::make_unique<OpenSim::Model>();
        rv->updDisplayHints().set_show_frames(true);
        return rv;
    }

    class UiModelStatePair final : public osc::VirtualModelStatePair {
    public:

        UiModelStatePair() :
            UiModelStatePair{makeNewModel()}
        {
        }

        UiModelStatePair(std::string const& osim) :
            UiModelStatePair{std::make_unique<OpenSim::Model>(osim)}
        {
        }

        UiModelStatePair(std::unique_ptr<OpenSim::Model> _model) :
            m_Model{std::move(_model)},
            m_ModelVersion{},
            m_FixupScaleFactor{1.0f},
            m_MaybeSelected{},
            m_MaybeHovered{}
        {
            osc::InitializeModel(*m_Model);
            osc::InitializeState(*m_Model);
        }

        UiModelStatePair(UiModelStatePair const& other) :
            m_Model{std::make_unique<OpenSim::Model>(*other.m_Model)},
            m_ModelVersion{},
            m_FixupScaleFactor{other.m_FixupScaleFactor},
            m_MaybeSelected{other.m_MaybeSelected},
            m_MaybeHovered{other.m_MaybeHovered}
        {
            osc::InitializeModel(*m_Model);
            osc::InitializeState(*m_Model);
        }

        UiModelStatePair(UiModelStatePair&&) noexcept = default;
        UiModelStatePair& operator=(UiModelStatePair const&) = delete;
        UiModelStatePair& operator=(UiModelStatePair&&) noexcept = default;
        ~UiModelStatePair() noexcept = default;

        OpenSim::Model const& implGetModel() const final
        {
            return *m_Model;
        }

        OpenSim::Model& updModel()
        {
            m_ModelVersion = osc::UID{};
            return *m_Model;
        }

        osc::UID implGetModelVersion() const final
        {
            return m_ModelVersion;
        }

        void setModelVersion(osc::UID version)
        {
            m_ModelVersion = version;
        }

        SimTK::State const& implGetState() const final
        {
            return m_Model->getWorkingState();
        }

        osc::UID implGetStateVersion() const final
        {
            return m_ModelVersion;
        }

        float implGetFixupScaleFactor() const final
        {
            return m_FixupScaleFactor;
        }

        void implSetFixupScaleFactor(float sf) final
        {
            m_FixupScaleFactor = sf;
        }

        OpenSim::ComponentPath const& getSelectedPath() const
        {
            return m_MaybeSelected;
        }

        void setSelectedPath(OpenSim::ComponentPath const& p)
        {
            m_MaybeSelected = p;
        }

        OpenSim::Component const* implGetSelected() const final
        {
            return osc::FindComponent(*m_Model, m_MaybeSelected);
        }

        void implSetSelected(OpenSim::Component const* c) final
        {
            m_MaybeSelected = osc::GetAbsolutePathOrEmpty(c);
        }

        OpenSim::ComponentPath const& getHoveredPath() const
        {
            return m_MaybeHovered;
        }

        void setHoveredPath(OpenSim::ComponentPath const& p)
        {
            m_MaybeHovered = p;
        }

        OpenSim::Component const* implGetHovered() const final
        {
            return osc::FindComponent(*m_Model, m_MaybeHovered);
        }

        void implSetHovered(OpenSim::Component const* c) final
        {
            m_MaybeHovered = osc::GetAbsolutePathOrEmpty(c);
        }

    private:
        // the model, finalized from its properties
        std::unique_ptr<OpenSim::Model> m_Model;
        osc::UID m_ModelVersion;

        // fixup scale factor of the model
        //
        // this scales up/down the decorations of the model - used for extremely
        // undersized models (e.g. fly leg)
        float m_FixupScaleFactor;

        // (maybe) absolute path to the current selection (empty otherwise)
        OpenSim::ComponentPath m_MaybeSelected;

        // (maybe) absolute path to the current hover (empty otherwise)
        OpenSim::ComponentPath m_MaybeHovered;
    };

    void CopySelectedAndHovered(UiModelStatePair const& src, UiModelStatePair& dest)
    {
        dest.setSelectedPath(src.getSelectedPath());
        dest.setHoveredPath(src.getHoveredPath());
    }
}

class osc::UndoableModelStatePair::Impl final {
public:

    Impl()
    {
        doCommit("created a new model");  // make initial commit
    }

    // crete a new commit graph that contains a backup of the given model
    explicit Impl(std::unique_ptr<OpenSim::Model> m) :
        m_Scratch{std::move(m)},
        m_MaybeFilesystemLocation{TryFindInputFile(m_Scratch.getModel())}
    {
        std::stringstream ss;
        if (!m_MaybeFilesystemLocation.empty())
        {
            ss << "loaded " << m_MaybeFilesystemLocation.filename().string();
        }
        else
        {
            ss << "loaded model";
        }
        doCommit(std::move(ss).str());  // make initial commit
    }

    explicit Impl(std::filesystem::path const& osimPath) :
        Impl{std::make_unique<OpenSim::Model>(osimPath.string())}
    {
        setUpToDateWithFilesystem(std::filesystem::last_write_time(osimPath));
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

    void setUpToDateWithFilesystem(std::filesystem::file_time_type t)
    {
        m_MaybeFilesystemTimestamp = std::move(t);
        m_MaybeCommitSavedToDisk = m_CurrentHead;
    }

    std::filesystem::file_time_type getLastFilesystemWriteTime() const
    {
        return m_MaybeFilesystemTimestamp;
    }

    ModelStateCommit const& getLatestCommit() const
    {
        return getHeadCommit();
    }

    bool canUndo() const
    {
        ModelStateCommit const* c = tryGetCommitByID(m_CurrentHead);
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
        // ensure the scratch space is clean
        try
        {
            OSC_PERF("commit model");
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
        checkout();  // care: skip copying selection because a rollback is aggro
    }

    bool tryCheckout(ModelStateCommit const& commit)
    {
        auto it = m_Commits.find(commit.getID());

        if (it == m_Commits.end())
        {
            return false;  // commit isn't in this model's storage (is it from another model?)
        }

        m_CurrentHead = commit.getID();
        checkout();
        return true;
    }

    OpenSim::Model const& getModel() const
    {
        return m_Scratch.getModel();
    }

    OpenSim::Model& updModel()
    {
        return m_Scratch.updModel();
    }

    void setModel(std::unique_ptr<OpenSim::Model> newModel)
    {
        UiModelStatePair p{std::move(newModel)};
        CopySelectedAndHovered(m_Scratch, p);
        m_Scratch = std::move(p);
    }

    UID getModelVersion() const
    {
        return m_Scratch.getModelVersion();
    }

    void setModelVersion(UID version)
    {
        m_Scratch.setModelVersion(version);
    }

    SimTK::State const& getState() const
    {
        return m_Scratch.getState();
    }

    UID getStateVersion() const
    {
        return m_Scratch.getStateVersion();
    }

    float getFixupScaleFactor() const
    {
        return m_Scratch.getFixupScaleFactor();
    }

    void setFixupScaleFactor(float v)
    {
        m_Scratch.setFixupScaleFactor(v);
    }

    OpenSim::Component const* getSelected() const
    {
        return m_Scratch.getSelected();
    }

    void setSelected(OpenSim::Component const* c)
    {
        m_Scratch.setSelected(c);
    }

    OpenSim::Component const* getHovered() const
    {
        return m_Scratch.getHovered();
    }

    void setHovered(OpenSim::Component const* c)
    {
        m_Scratch.setHovered(c);
    }

private:

    UID doCommit(std::string_view message)
    {
        auto commit = ModelStateCommit{m_Scratch, std::move(message), m_CurrentHead};
        UID commitID = commit.getID();

        m_Commits.try_emplace(commitID, std::move(commit));
        m_CurrentHead = commitID;
        m_BranchHead = commitID;

        garbageCollect();

        return commitID;
    }

    // try to lookup a commit by its ID
    ModelStateCommit const* tryGetCommitByID(UID id) const
    {
        auto it = m_Commits.find(id);
        return it != m_Commits.end() ? &it->second : nullptr;
    }

    ModelStateCommit const& getHeadCommit() const
    {
        OSC_ASSERT(m_CurrentHead != UID::empty());
        OSC_ASSERT(hasCommit(m_CurrentHead));

        return *tryGetCommitByID(m_CurrentHead);
    }

    // try to lookup the *parent* of a given commit, or return an empty (senteniel) ID
    UID tryGetParentIDOrEmpty(UID id) const
    {
        ModelStateCommit const* commit = tryGetCommitByID(id);
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
    ModelStateCommit const* nthAncestor(UID a, int n) const
    {
        if (n < 0)
        {
            return nullptr;
        }

        ModelStateCommit const* c = tryGetCommitByID(a);

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
        ModelStateCommit const* c = nthAncestor(a, n);
        return c ? c->getID() : UID::empty();
    }

    // returns `true` if `maybeAncestor` is an ancestor of `id`
    bool isAncestor(UID maybeAncestor, UID id)
    {
        ModelStateCommit const* c = tryGetCommitByID(id);

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
        static_assert(c_MaxUndo >= 0);

        UID firstBadCommitIDOrEmpty = nthAncestorID(m_CurrentHead, c_MaxUndo + 1);
        eraseCommitRange(firstBadCommitIDOrEmpty, UID::empty());
    }

    // garbage collect (erase) commits that fall outside the maximum redo depth
    void garbageCollectMaxRedo()
    {
        static_assert(c_MaxRedo >= 0);

        int numRedos = distance(m_BranchHead, m_CurrentHead);
        int numDeletions = numRedos - c_MaxRedo;

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

        ModelStateCommit const* c = tryGetCommitByID(m_CurrentHead);

        if (c)
        {
            UiModelStatePair newScratch{std::make_unique<OpenSim::Model>(*c->getModel())};
            CopySelectedAndHovered(m_Scratch, newScratch);
            newScratch.setFixupScaleFactor(m_Scratch.getFixupScaleFactor());
            m_Scratch = std::move(newScratch);
        }
    }

    // performs an undo, if possible
    //
    // effectively, checks out HEAD~1
    void undo()
    {
        ModelStateCommit const* c = tryGetCommitByID(m_CurrentHead);

        if (!c)
        {
            return;
        }

        ModelStateCommit const* parent = tryGetCommitByID(c->getParentID());

        if (!parent)
        {
            return;
        }

        // perform hacky fixups to ensure the user experience is best:
        //
        // - user's selection state should be "sticky" between undo/redo
        // - user's scene scale factor should be "sticky" between undo/redo
        UiModelStatePair newModel{std::make_unique<OpenSim::Model>(*parent->getModel())};
        CopySelectedAndHovered(m_Scratch, newModel);
        newModel.setFixupScaleFactor(m_Scratch.getFixupScaleFactor());

        m_Scratch = std::move(newModel);
        m_CurrentHead = parent->getID();
    }

    // performs a redo, if possible
    void redo()
    {
        int dist = distance(m_BranchHead, m_CurrentHead);

        if (dist < 1)
        {
            return;
        }

        ModelStateCommit const* c = nthAncestor(m_BranchHead, dist - 1);

        if (!c)
        {
            return;
        }

        // perform hacky fixups to ensure the user experience is best:
        //
        // - user's selection state should be "sticky" between undo/redo
        // - user's scene scale factor should be "sticky" between undo/redo
        UiModelStatePair newModel{std::make_unique<OpenSim::Model>(*c->getModel())};
        CopySelectedAndHovered(m_Scratch, newModel);
        newModel.setFixupScaleFactor(m_Scratch.getFixupScaleFactor());

        m_Scratch = std::move(newModel);
        m_CurrentHead = c->getID();
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

private:
    // mutable staging area that calling code can mutate
    UiModelStatePair m_Scratch;

    // where scratch will commit to (i.e. the parent of the scratch area)
    UID m_CurrentHead = UID::empty();

    // head of the current branch (i.e. "main") - may be ahead of current branch (undo/redo)
    UID m_BranchHead = UID::empty();

    // underlying storage for immutable commits
    std::unordered_map<UID, ModelStateCommit> m_Commits;

    // (maybe) the location of the model on-disk
    std::filesystem::path m_MaybeFilesystemLocation;

    // the timestamp of the on-disk data (needed to know when to trigger a reload)
    std::filesystem::file_time_type m_MaybeFilesystemTimestamp;

    // (maybe) the version of the model that was last saved to disk
    UID m_MaybeCommitSavedToDisk = UID::empty();
};


// public API (PIMPL)

osc::UndoableModelStatePair::UndoableModelStatePair() :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::UndoableModelStatePair::UndoableModelStatePair(std::unique_ptr<OpenSim::Model> model) :
    m_Impl{std::make_unique<Impl>(std::move(model))}
{
}

osc::UndoableModelStatePair::UndoableModelStatePair(std::filesystem::path const& osimPath) :
    m_Impl{std::make_unique<Impl>(osimPath)}
{
}

osc::UndoableModelStatePair::UndoableModelStatePair(UndoableModelStatePair const& src) :
    m_Impl{std::make_unique<Impl>(*src.m_Impl)}
{
}

osc::UndoableModelStatePair::UndoableModelStatePair(UndoableModelStatePair&&) noexcept = default;

osc::UndoableModelStatePair& osc::UndoableModelStatePair::operator=(UndoableModelStatePair const& src)
{
    if (&src != this)
    {
        std::unique_ptr<Impl> cpy = std::make_unique<Impl>(*src.m_Impl);
        std::swap(m_Impl, cpy);
    }

    return *this;
}

osc::UndoableModelStatePair& osc::UndoableModelStatePair::operator=(UndoableModelStatePair&&) noexcept = default;
osc::UndoableModelStatePair::~UndoableModelStatePair() noexcept = default;

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

void osc::UndoableModelStatePair::setUpToDateWithFilesystem(std::filesystem::file_time_type t)
{
    m_Impl->setUpToDateWithFilesystem(std::move(t));
}

std::filesystem::file_time_type osc::UndoableModelStatePair::getLastFilesystemWriteTime() const
{
    return m_Impl->getLastFilesystemWriteTime();
}


osc::ModelStateCommit const& osc::UndoableModelStatePair::getLatestCommit() const
{
    return m_Impl->getLatestCommit();
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

bool osc::UndoableModelStatePair::tryCheckout(ModelStateCommit const& commit)
{
    return m_Impl->tryCheckout(commit);
}

OpenSim::Model& osc::UndoableModelStatePair::updModel()
{
    return m_Impl->updModel();
}

void osc::UndoableModelStatePair::setModel(std::unique_ptr<OpenSim::Model> newModel)
{
    m_Impl->setModel(std::move(newModel));
}

void osc::UndoableModelStatePair::setModelVersion(UID version)
{
    m_Impl->setModelVersion(version);
}

OpenSim::Model const& osc::UndoableModelStatePair::implGetModel() const
{
    return m_Impl->getModel();
}

osc::UID osc::UndoableModelStatePair::implGetModelVersion() const
{
    return m_Impl->getModelVersion();
}

SimTK::State const& osc::UndoableModelStatePair::implGetState() const
{
    return m_Impl->getState();
}

osc::UID osc::UndoableModelStatePair::implGetStateVersion() const
{
    return m_Impl->getStateVersion();
}

float osc::UndoableModelStatePair::implGetFixupScaleFactor() const
{
    return m_Impl->getFixupScaleFactor();
}

void osc::UndoableModelStatePair::implSetFixupScaleFactor(float v)
{
    m_Impl->setFixupScaleFactor(std::move(v));
}

OpenSim::Component const* osc::UndoableModelStatePair::implGetSelected() const
{
    return m_Impl->getSelected();
}

void osc::UndoableModelStatePair::implSetSelected(OpenSim::Component const* c)
{
    m_Impl->setSelected(std::move(c));
}

OpenSim::Component const* osc::UndoableModelStatePair::implGetHovered() const
{
    return m_Impl->getHovered();
}

void osc::UndoableModelStatePair::implSetHovered(OpenSim::Component const* c)
{
    m_Impl->setHovered(std::move(c));
}

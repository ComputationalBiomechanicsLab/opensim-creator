#include "UiModel.hpp"

#include "src/3D/Model.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/RenderableScene.hpp"
#include "src/OpenSimBindings/StateModifications.hpp"
#include "src/Utils/Perf.hpp"
#include "src/Log.hpp"
#include "src/os.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentPath.h>

#include <memory>
#include <vector>

using namespace osc;

static std::unique_ptr<OpenSim::Model> makeNewModel()
{
    auto rv = std::make_unique<OpenSim::Model>();
    rv->updDisplayHints().set_show_frames(true);
    return rv;
}

class osc::UiModel::Impl final {
public:

    Impl() :
        Impl{makeNewModel()}
    {
    }

    Impl(std::string const& osim) :
        Impl{std::make_unique<OpenSim::Model>(osim)}
    {
    }

    Impl(Impl const& old, std::unique_ptr<OpenSim::Model> model) :
        m_StateModifications{old.m_StateModifications},
        m_Model{std::move(model)},
        m_Decorations{},
        m_SceneBVH{},
        m_FixupScaleFactor{old.m_FixupScaleFactor},
        m_MaybeSelected{old.m_MaybeSelected},
        m_MaybeHovered{old.m_MaybeHovered},
        m_MaybeIsolated{old.m_MaybeIsolated},
        m_LastModified{old.m_LastModified},
        m_ModelIsDirty{true},
        m_StateIsDirty{true},
        m_DecorationsAreDirty{true},
        m_FakeDirty{true}
    {
    }

    Impl(std::unique_ptr<OpenSim::Model> _model) :
        m_StateModifications{},
        m_Model{std::move(_model)},
        m_Decorations{},
        m_SceneBVH{},
        m_FixupScaleFactor{1.0f},
        m_MaybeSelected{},
        m_MaybeHovered{},
        m_MaybeIsolated{},
        m_LastModified{std::chrono::system_clock::now()},
        m_ModelIsDirty{true},
        m_StateIsDirty{true},
        m_DecorationsAreDirty{true},
        m_FakeDirty{true}
    {
    }

    Impl(Impl const& other) :
        m_StateModifications{other.m_StateModifications},
        m_Model{std::make_unique<OpenSim::Model>(*other.m_Model)},
        m_Decorations{},
        m_SceneBVH{},
        m_FixupScaleFactor{other.m_FixupScaleFactor},
        m_MaybeSelected{other.m_MaybeSelected},
        m_MaybeHovered{other.m_MaybeHovered},
        m_MaybeIsolated{other.m_MaybeIsolated},
        m_LastModified{other.m_LastModified},
        m_ModelIsDirty{true},
        m_StateIsDirty{true},
        m_DecorationsAreDirty{true},
        m_FakeDirty{true}
    {
    }

    Impl(Impl&&) noexcept = default;

    ~Impl() noexcept = default;

    Impl& operator=(Impl const&) = delete;
    Impl& operator=(Impl&&) noexcept = default;

    std::unique_ptr<Impl> clone()
    {
        return std::make_unique<Impl>(*this);
    }

    OpenSim::Model const& getModel() const
    {
        // HACK: ensure the user can't get access to a dirty model/system/state
        {
            bool wasDirty = isDirty();
            const_cast<Impl&>(*this).updateIfDirty();
            const_cast<Impl&>(*this).m_FakeDirty = wasDirty;
        }

        return *m_Model;
    }

    OpenSim::Model& updModel()
    {
        // HACK: ensure the user can't get access to a dirty model/system/state
        {
            bool wasDirty = isDirty();
            const_cast<Impl&>(*this).updateIfDirty();
            m_FakeDirty = wasDirty;
        }
        setDirty(true);
        return *m_Model;
    }

    void setModel(std::unique_ptr<OpenSim::Model> m)
    {
        *this = Impl{*this, std::move(m)};
    }

    SimTK::State const& getState() const
    {
        // HACK: ensure the user can't get access to a dirty model/system/state
        {
            bool wasDirty = isDirty();
            const_cast<Impl&>(*this).updateIfDirty();
            const_cast<Impl&>(*this).m_FakeDirty = wasDirty;
        }
        return m_Model->getWorkingState();
    }

    StateModifications const& getStateModifications() const
    {
        return m_StateModifications;
    }

    void pushCoordinateEdit(OpenSim::Coordinate const& c, CoordinateEdit const& ce)
    {
        m_StateModifications.pushCoordinateEdit(c, ce);

        setStateDirtyADVANCED(true);
        setDecorationsDirtyADVANCED(true);
    }

    bool removeCoordinateEdit(OpenSim::Coordinate const& c)
    {
        if (m_StateModifications.removeCoordinateEdit(c))
        {
            setStateDirtyADVANCED(true);
            setDecorationsDirtyADVANCED(true);
            return true;
        }
        else
        {
            return false;
        }
    }

    nonstd::span<ComponentDecoration const> getSceneDecorations() const
    {
        // HACK: ensure the user can't get access to a dirty model/system/state
        {
            bool wasDirty = isDirty();
            const_cast<Impl&>(*this).updateIfDirty();
            const_cast<Impl&>(*this).m_FakeDirty = wasDirty;
        }
        return m_Decorations;
    }

    BVH const& getSceneBVH() const
    {
        // HACK: ensure the user can't get access to a dirty model/system/state
        {
            bool wasDirty = isDirty();
            const_cast<Impl&>(*this).updateIfDirty();
            const_cast<Impl&>(*this).m_FakeDirty = wasDirty;
        }
        return m_SceneBVH;
    }

    float getFixupScaleFactor() const
    {
        return m_FixupScaleFactor;
    }

    void setFixupScaleFactor(float sf)
    {
        m_FixupScaleFactor = sf;
        setDecorationsDirtyADVANCED(true);
    }

    AABB getSceneAABB() const
    {
        auto const& bvh = getSceneBVH();
        if (!bvh.nodes.empty())
        {
            return bvh.nodes[0].bounds;
        }
        else
        {
            return AABB{};
        }
    }

    glm::vec3 getSceneDimensions() const
    {
        return AABBDims(getSceneAABB());
    }

    float getSceneLongestDimension() const
    {
        return AABBLongestDim(getSceneAABB());
    }

    float getRecommendedScaleFactor() const
    {
        // generate decorations as if they were empty-sized and union their
        // AABBs to get an idea of what the "true" scale of the model probably
        // is (without the model containing oversized frames, etc.)
        std::vector<ComponentDecoration> ses;
        GenerateModelDecorations(getModel(), getState(), 0.0f, ses, getSelected(), getHovered());

        if (ses.empty())
        {
            return 1.0f;
        }

        AABB aabb = ses[0].worldspaceAABB;
        for (size_t i = 1; i < ses.size(); ++i)
        {
            aabb = AABBUnion(aabb, ses[i].worldspaceAABB);
        }

        float longest = AABBLongestDim(aabb);
        float rv = 1.0f;

        while (longest < 0.1)
        {
            longest *= 10.0f;
            rv /= 10.0f;
        }

        return rv;
    }

    bool isDirty() const
    {
        return m_ModelIsDirty || m_StateIsDirty || m_DecorationsAreDirty || m_FakeDirty;
    }

    void setModelDirtyADVANCED(bool v)
    {
        if (!m_ModelIsDirty && v)
        {
            log::debug("model dirtying event happened");
        }

        m_ModelIsDirty = v;

        if (v)
        {
            m_LastModified = std::chrono::system_clock::now();
        }
    }

    void setStateDirtyADVANCED(bool v)
    {
        if (!m_StateIsDirty && v)
        {
            log::debug("state dirtying event happened");
        }

        m_StateIsDirty = v;

        if (v)
        {
            m_LastModified = std::chrono::system_clock::now();
        }
    }

    void setDecorationsDirtyADVANCED(bool v)
    {
        if (!m_DecorationsAreDirty && v)
        {
            log::debug("decoration dirtying event happened");
        }

        m_DecorationsAreDirty = v;

        if (v)
        {
            m_LastModified = std::chrono::system_clock::now();
        }
    }

    void setDirty(bool v)
    {
        setModelDirtyADVANCED(v);
        setStateDirtyADVANCED(v);
        setDecorationsDirtyADVANCED(v);
    }

    void updateIfDirty()
    {
        if (m_ModelIsDirty)
        {
            OSC_PERF("model update");

            m_Model->finalizeFromProperties();
            m_Model->finalizeConnections();
            m_Model->buildSystem();
            m_Model->initializeState();
            m_ModelIsDirty = false;
        }

        if (m_StateIsDirty)
        {
            OSC_PERF("state update");

            {
                OSC_PERF("apply state modifications");
                m_StateModifications.applyToState(*m_Model, m_Model->updWorkingState());
            }

            {
                OSC_PERF("equilibriate muscles");
                m_Model->equilibrateMuscles(m_Model->updWorkingState());
            }

            {
                OSC_PERF("realize velocity");
                m_Model->realizeVelocity(m_Model->updWorkingState());
            }
            m_StateIsDirty = false;
        }

        if (m_DecorationsAreDirty)
        {
            OSC_PERF("decoration update");

            {
                OSC_PERF("generate decorations");
                GenerateModelDecorations(*m_Model, m_Model->updWorkingState(), m_FixupScaleFactor, m_Decorations, getSelected(), getHovered());
            }

            {
                OSC_PERF("generate BVH");
                UpdateSceneBVH(m_Decorations, m_SceneBVH);
            }
            m_DecorationsAreDirty = false;
        }

        m_FakeDirty = false;
    }

    bool hasSelected() const
    {
        return FindComponent(*m_Model, m_MaybeSelected);
    }

    OpenSim::Component const* getSelected() const
    {
        return FindComponent(*m_Model, m_MaybeSelected);
    }

    OpenSim::Component* updSelected()
    {
        setDirty(true);
        return FindComponentMut(*m_Model, m_MaybeSelected);
    }

    void setSelected(OpenSim::Component const* c)
    {
        if (c)
        {
            m_MaybeSelected = c->getAbsolutePath();
        }
        else
        {
            m_MaybeSelected = {};
        }
    }

    bool selectionHasTypeHashCode(size_t v) const
    {
        OpenSim::Component const* selected = getSelected();
        return selected && typeid(*selected).hash_code() == v;
    }

    bool hasHovered() const
    {
        return FindComponent(*m_Model, m_MaybeHovered);
    }

    OpenSim::Component const* getHovered() const
    {
        return FindComponent(*m_Model, m_MaybeHovered);
    }

    OpenSim::Component* updHovered()
    {
        setDirty(true);
        return FindComponentMut(*m_Model, m_MaybeHovered);
    }

    void setHovered(OpenSim::Component const* c)
    {
        if (c)
        {
            m_MaybeHovered = c->getAbsolutePath();
        }
        else
        {
            m_MaybeHovered = {};
        }
    }

    OpenSim::Component const* getIsolated() const
    {
        return FindComponent(*m_Model, m_MaybeIsolated);
    }

    OpenSim::Component* updIsolated()
    {
        setDirty(true);
        return FindComponentMut(*m_Model, m_MaybeIsolated);
    }

    void setIsolated(OpenSim::Component const* c)
    {
        if (c)
        {
            m_MaybeIsolated = c->getAbsolutePath();
        }
        else
        {
            m_MaybeIsolated = {};
        }
    }

    void setSelectedHoveredAndIsolatedFrom(Impl const& other)
    {
        m_MaybeSelected = other.m_MaybeSelected;
        m_MaybeHovered = other.m_MaybeHovered;
        m_MaybeIsolated = other.m_MaybeIsolated;
    }

    void declareDeathOf(OpenSim::Component const* c)
    {
        if (getSelected() == c)
        {
            setSelected(nullptr);
        }

        if (getHovered() == c)
        {
            setHovered(nullptr);
        }

        if (getIsolated() == c)
        {
            setIsolated(nullptr);
        }
    }

    std::chrono::system_clock::time_point getLastModifiedTime() const
    {
        return m_LastModified;
    }

private:
    // user-enacted state modifications (e.g. coordinate edits)
    StateModifications m_StateModifications;

    // the model, finalized from its properties
    std::unique_ptr<OpenSim::Model> m_Model;

    // decorations, generated from model's display properties etc.
    std::vector<ComponentDecoration> m_Decorations;

    // scene-level BVH of decoration AABBs
    BVH m_SceneBVH;

    // fixup scale factor of the model
    //
    // this scales up/down the decorations of the model - used for extremely
    // undersized models (e.g. fly leg)
    float m_FixupScaleFactor;

    // (maybe) absolute path to the current selection (empty otherwise)
    OpenSim::ComponentPath m_MaybeSelected;

    // (maybe) absolute path to the current hover (empty otherwise)
    OpenSim::ComponentPath m_MaybeHovered;

    // (maybe) absolute path to the current isolation (empty otherwise)
    OpenSim::ComponentPath m_MaybeIsolated;

    // generic timestamp
    //
    // can indicate creation or latest modification, it's here to roughly
    // track how old/new the instance is
    std::chrono::system_clock::time_point m_LastModified;

    bool m_ModelIsDirty;
    bool m_StateIsDirty;
    bool m_DecorationsAreDirty;
    bool m_FakeDirty;  // "pretends" the model was dirty - used by calling code to detect dirtiness
};

osc::UiModel::UiModel() :
    m_Impl{new Impl{}}
{
}

osc::UiModel::UiModel(std::string const& osim) :
    m_Impl{new Impl{osim}}
{
}

osc::UiModel::UiModel(std::unique_ptr<OpenSim::Model> _model) :
    m_Impl{new Impl{std::move(_model)}}
{
}

osc::UiModel::UiModel(UiModel const& other) :
    m_Impl{new Impl{*other.m_Impl}}
{
}

osc::UiModel::UiModel(UiModel&&) noexcept = default;

osc::UiModel::~UiModel() noexcept = default;

UiModel& osc::UiModel::operator=(UiModel const& other) = default;

osc::UiModel& osc::UiModel::operator=(UiModel&&) noexcept = default;


OpenSim::Model const& osc::UiModel::getModel() const
{
    return m_Impl->getModel();
}

OpenSim::Model& osc::UiModel::updModel()
{
    return m_Impl->updModel();
}

void osc::UiModel::setModel(std::unique_ptr<OpenSim::Model> m)
{
    m_Impl->setModel(std::move(m));
}


SimTK::State const& osc::UiModel::getState() const
{
    return m_Impl->getState();
}

StateModifications const& osc::UiModel::getStateModifications() const
{
    return m_Impl->getStateModifications();
}

void osc::UiModel::pushCoordinateEdit(OpenSim::Coordinate const& c, CoordinateEdit const& ce)
{
    m_Impl->pushCoordinateEdit(c, ce);
}

bool osc::UiModel::removeCoordinateEdit(OpenSim::Coordinate const& c)
{
    return m_Impl->removeCoordinateEdit(c);
}

nonstd::span<ComponentDecoration const> osc::UiModel::getSceneDecorations() const
{
    return m_Impl->getSceneDecorations();
}

osc::BVH const& osc::UiModel::getSceneBVH() const
{
    return m_Impl->getSceneBVH();
}

float osc::UiModel::getFixupScaleFactor() const
{
    return m_Impl->getFixupScaleFactor();
}

void osc::UiModel::setFixupScaleFactor(float sf)
{
    m_Impl->setFixupScaleFactor(std::move(sf));
}

AABB osc::UiModel::getSceneAABB() const
{
    return m_Impl->getSceneAABB();
}

glm::vec3 osc::UiModel::getSceneDimensions() const
{
    return m_Impl->getSceneDimensions();
}

float osc::UiModel::getSceneLongestDimension() const
{
    return m_Impl->getSceneLongestDimension();
}

float osc::UiModel::getRecommendedScaleFactor() const
{
    return m_Impl->getRecommendedScaleFactor();
}

bool osc::UiModel::isDirty() const
{
    return m_Impl->isDirty();
}

void osc::UiModel::setModelDirtyADVANCED(bool v)
{
    m_Impl->setModelDirtyADVANCED(std::move(v));
}

void osc::UiModel::setStateDirtyADVANCED(bool v)
{
    m_Impl->setStateDirtyADVANCED(std::move(v));
}

void osc::UiModel::setDecorationsDirtyADVANCED(bool v)
{
    m_Impl->setDecorationsDirtyADVANCED(std::move(v));
}

void osc::UiModel::setDirty(bool v)
{
    m_Impl->setDirty(std::move(v));
}

void osc::UiModel::updateIfDirty()
{
    m_Impl->updateIfDirty();
}

bool osc::UiModel::hasSelected() const
{
    return m_Impl->hasSelected();
}

OpenSim::Component const* osc::UiModel::getSelected() const
{
    return m_Impl->getSelected();
}

OpenSim::Component* osc::UiModel::updSelected()
{
    return m_Impl->updSelected();
}

void osc::UiModel::setSelected(OpenSim::Component const* c)
{
    m_Impl->setSelected(std::move(c));
}

bool osc::UiModel::selectionHasTypeHashCode(size_t v) const
{
    return m_Impl->selectionHasTypeHashCode(std::move(v));
}

bool osc::UiModel::hasHovered() const
{
    return m_Impl->hasHovered();
}

OpenSim::Component const* osc::UiModel::getHovered() const
{
    return m_Impl->getHovered();
}

OpenSim::Component* osc::UiModel::updHovered()
{
    return m_Impl->updHovered();
}

void osc::UiModel::setHovered(OpenSim::Component const* c)
{
    m_Impl->setHovered(std::move(c));
}

OpenSim::Component const* osc::UiModel::getIsolated() const
{
    return m_Impl->getIsolated();
}

OpenSim::Component* osc::UiModel::updIsolated()
{
    return m_Impl->updIsolated();
}

void osc::UiModel::setIsolated(OpenSim::Component const* c)
{
    m_Impl->setIsolated(std::move(c));
}

void osc::UiModel::setSelectedHoveredAndIsolatedFrom(UiModel const& uim)
{
    m_Impl->setSelectedHoveredAndIsolatedFrom(*uim.m_Impl);
}

void osc::UiModel::declareDeathOf(OpenSim::Component const* c)
{
    m_Impl->declareDeathOf(std::move(c));
}


std::chrono::system_clock::time_point osc::UiModel::getLastModifiedTime() const
{
    return m_Impl->getLastModifiedTime();
}

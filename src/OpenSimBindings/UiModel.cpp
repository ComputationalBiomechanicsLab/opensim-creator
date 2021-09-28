#include "UiModel.hpp"

#include "src/3D/Model.hpp"
#include "src/OpenSimBindings/RenderableScene.hpp"
#include "src/OpenSimBindings/StateModifications.hpp"
#include "src/Log.hpp"
#include "src/os.hpp"

#include <OpenSim/Simulation/Model/Model.h>

#include <memory>
#include <vector>

using namespace osc;

// translate a pointer to a component in model A to a pointer to a component in model B
//
// returns nullptr if the pointer cannot be cleanly translated
static OpenSim::Component const* findComponent(OpenSim::Model const& model, OpenSim::ComponentPath const& absPath) {
    for (OpenSim::Component const& c : model.getComponentList()) {
        if (c.getAbsolutePath() == absPath) {
            return &c;
        }
    }
    return nullptr;
}

static OpenSim::Component* relocateComponentPointerToAnotherModel(OpenSim::Model const& model, OpenSim::Component const* ptr) {
    if (!ptr) {
        return nullptr;
    }

    return const_cast<OpenSim::Component*>(findComponent(model, ptr->getAbsolutePath()));
}

static std::unique_ptr<SimTK::State> initializeState(OpenSim::Model& m, StateModifications& modifications) {
    std::unique_ptr<SimTK::State> rv = std::make_unique<SimTK::State>(m.initializeState());
    modifications.applyToState(m, *rv);
    m.equilibrateMuscles(*rv);
    m.realizePosition(*rv);
    return rv;
}

static std::unique_ptr<OpenSim::Model> makeNewModel() {
    auto rv = std::make_unique<OpenSim::Model>();
    rv->updDisplayHints().set_show_frames(true);
    return rv;
}

static void announceDirtyingEvent() {
    log::info("model dirtied");
    //WriteTracebackToLog(log::level::info);
}

struct osc::UiModel::Impl final {
    // user-enacted state modifications (e.g. coordinate edits)
    StateModifications m_StateModifications;

    // the model, finalized from its properties
    std::unique_ptr<OpenSim::Model> m_Model;

    // SimTK::State, in a renderable state (e.g. realized up to a relevant stage)
    std::unique_ptr<SimTK::State> m_State;

    // decorations, generated from model's display properties etc.
    std::vector<LabelledSceneElement> m_Decorations;

    // scene-level BVH of decoration AABBs
    BVH m_SceneBVH;

    // fixup scale factor of the model
    //
    // this scales up/down the decorations of the model - used for extremely
    // undersized models (e.g. fly leg)
    float m_FixupScaleFactor;

    // current selection, if any
    OpenSim::Component* m_CurrentSelection;

    // current hover, if any
    OpenSim::Component* m_Hovered;

    // current isolation, if any
    //
    // "isolation" here means that the user is only interested in this
    // particular subcomponent in the model, so visualizers etc. should
    // try to only show that component
    OpenSim::Component* m_Isolated;

    // generic timestamp
    //
    // can indicate creation or latest modification, it's here to roughly
    // track how old/new the instance is
    std::chrono::system_clock::time_point m_LastModified;

    // set to true when the user potentially modified the model
    bool m_IsDirty;

    Impl() : Impl{makeNewModel()} {
    }

    Impl(std::string const& osim) : Impl{std::make_unique<OpenSim::Model>(osim)} {

    }

    Impl(std::unique_ptr<OpenSim::Model> _model) :
        m_StateModifications{},
        m_Model{[&_model]() {
            _model->finalizeFromProperties();
            _model->finalizeConnections();
            _model->buildSystem();
            return std::unique_ptr<OpenSim::Model>{std::move(_model)};
        }()},
        m_State{initializeState(*m_Model, m_StateModifications)},
        m_Decorations{},
        m_SceneBVH{},
        m_FixupScaleFactor{1.0f},
        m_CurrentSelection{nullptr},
        m_Hovered{nullptr},
        m_Isolated{nullptr},
        m_LastModified{std::chrono::system_clock::now()},
        m_IsDirty{false} {

        generateDecorations(*m_Model, *m_State, m_FixupScaleFactor, m_Decorations);
        updateBVH(m_Decorations, m_SceneBVH);
    }

    Impl(Impl const& other) :
        m_StateModifications{other.m_StateModifications},
        m_Model{[&other]() {
            auto copy = std::make_unique<OpenSim::Model>(*other.m_Model);
            copy->finalizeFromProperties();
            copy->finalizeConnections();
            copy->buildSystem();
            return copy;
        }()},
        m_State{initializeState(*m_Model, m_StateModifications)},
        m_Decorations{},
        m_SceneBVH{},
        m_FixupScaleFactor{other.m_FixupScaleFactor},
        m_CurrentSelection{relocateComponentPointerToAnotherModel(*m_Model, other.m_CurrentSelection)},
        m_Hovered{relocateComponentPointerToAnotherModel(*m_Model, other.m_Hovered)},
        m_Isolated{relocateComponentPointerToAnotherModel(*m_Model, other.m_Isolated)},
        m_LastModified{other.m_LastModified},
        m_IsDirty{false} {

        generateDecorations(*m_Model, *m_State, m_FixupScaleFactor, m_Decorations);
        updateBVH(m_Decorations, m_SceneBVH);
    }

    void setDirty(bool v) {
        if (!m_IsDirty && v) {
            log::info("dirtying event happened");
        }

        m_IsDirty = v;
        if (v) {
            m_LastModified = std::chrono::system_clock::now();
        }
    }
};

osc::UiModel::UiModel() : m_Impl{new Impl{}} {
}

osc::UiModel::UiModel(std::string const& osim) : m_Impl{new Impl{osim}} {
}

osc::UiModel::UiModel(std::unique_ptr<OpenSim::Model> _model) : m_Impl{new Impl{std::move(_model)}} {
}

osc::UiModel::UiModel(UiModel const& other) : m_Impl{new Impl{*other.m_Impl}} {
}

osc::UiModel::UiModel(UiModel&&) noexcept = default;

osc::UiModel::~UiModel() noexcept = default;

osc::UiModel& osc::UiModel::operator=(UiModel&&) = default;

osc::UiModel& osc::UiModel::operator=(UiModel const& other) {
    UiModel copy{other};
    *this = std::move(copy);
    return *this;
}

StateModifications const& osc::UiModel::getStateModifications() const {
    return m_Impl->m_StateModifications;
}

OpenSim::Model const& osc::UiModel::getModel() const {
    return *m_Impl->m_Model;
}

OpenSim::Model& osc::UiModel::updModel() {
    m_Impl->setDirty(true);
    return *m_Impl->m_Model;
}

void osc::UiModel::setModel(std::unique_ptr<OpenSim::Model> m) {
    auto newImpl = std::make_unique<Impl>(std::move(m));
    newImpl->m_CurrentSelection = relocateComponentPointerToAnotherModel(*newImpl->m_Model, m_Impl->m_CurrentSelection);
    newImpl->m_Hovered = relocateComponentPointerToAnotherModel(*newImpl->m_Model, m_Impl->m_Hovered);
    newImpl->m_Isolated = relocateComponentPointerToAnotherModel(*newImpl->m_Model, m_Impl->m_Isolated);
    m_Impl = std::move(newImpl);
}

SimTK::State const& osc::UiModel::getState() const {
    return *m_Impl->m_State;
}

SimTK::State& osc::UiModel::updState() {
    m_Impl->setDirty(true);
    return *m_Impl->m_State;
}

bool osc::UiModel::isDirty() const {
    return m_Impl->m_IsDirty;
}

void osc::UiModel::updateIfDirty() {
    if (!m_Impl->m_IsDirty) {
        return;
    }

    m_Impl->m_Model->finalizeFromProperties();
    m_Impl->m_Model->finalizeConnections();
    m_Impl->m_Model->buildSystem();
    m_Impl->m_State = initializeState(*m_Impl->m_Model, m_Impl->m_StateModifications);
    generateDecorations(*m_Impl->m_Model, *m_Impl->m_State, m_Impl->m_FixupScaleFactor, m_Impl->m_Decorations);
    updateBVH(m_Impl->m_Decorations, m_Impl->m_SceneBVH);
    m_Impl->setDirty(false);
}

void osc::UiModel::setModelDirty(bool v) {
    m_Impl->setDirty(v);
}

void osc::UiModel::setStateDirty(bool v) {
    m_Impl->setDirty(v);
}

void osc::UiModel::setDecorationsDirty(bool v) {
    m_Impl->setDirty(v);
}

void osc::UiModel::setAllDirty(bool v) {
    m_Impl->setDirty(v);
}

nonstd::span<LabelledSceneElement const> osc::UiModel::getSceneDecorations() const {
    return m_Impl->m_Decorations;
}

osc::BVH const& osc::UiModel::getSceneBVH() const {
    return m_Impl->m_SceneBVH;
}

float osc::UiModel::getFixupScaleFactor() const {
    return m_Impl->m_FixupScaleFactor;
}

void osc::UiModel::setFixupScaleFactor(float sf) {
    m_Impl->m_FixupScaleFactor = sf;
    generateDecorations(*m_Impl->m_Model, *m_Impl->m_State, m_Impl->m_FixupScaleFactor, m_Impl->m_Decorations);
    updateBVH(m_Impl->m_Decorations, m_Impl->m_SceneBVH);
}

bool osc::UiModel::hasSelected() const {
    return m_Impl->m_CurrentSelection != nullptr;
}

OpenSim::Component const* osc::UiModel::getSelected() const {
    return m_Impl->m_CurrentSelection;
}

OpenSim::Component* osc::UiModel::updSelected() {
    m_Impl->setDirty(true);
    return m_Impl->m_CurrentSelection;
}

void osc::UiModel::setSelected(OpenSim::Component const* c) {
    m_Impl->m_CurrentSelection = const_cast<OpenSim::Component*>(c);
}

bool osc::UiModel::selectionHasTypeHashCode(size_t v) const {
    return m_Impl->m_CurrentSelection && typeid(*m_Impl->m_CurrentSelection).hash_code() == v;
}

bool osc::UiModel::hasHovered() const {
    return m_Impl->m_Hovered != nullptr;
}

OpenSim::Component const* osc::UiModel::getHovered() const {
    return m_Impl->m_Hovered;
}

OpenSim::Component* osc::UiModel::updHovered() {
    m_Impl->setDirty(true);
    return m_Impl->m_Hovered;
}

void osc::UiModel::setHovered(OpenSim::Component const* c) {
    m_Impl->m_Hovered = const_cast<OpenSim::Component*>(c);
}

OpenSim::Component const* osc::UiModel::getIsolated() const {
    return m_Impl->m_Isolated;
}

OpenSim::Component* osc::UiModel::updIsolated() {
    m_Impl->setDirty(true);
    return m_Impl->m_Isolated;
}

void osc::UiModel::setIsolated(OpenSim::Component const* c) {
    m_Impl->m_Isolated = const_cast<OpenSim::Component*>(c);
}

void osc::UiModel::setSelectedHoveredAndIsolatedFrom(UiModel const& uim) {
    OpenSim::Component const* newSelection = relocateComponentPointerToAnotherModel(*m_Impl->m_Model, uim.getSelected());
    if (newSelection) {
        setSelected(newSelection);
    }

    OpenSim::Component const* newHover = relocateComponentPointerToAnotherModel(*m_Impl->m_Model, uim.getHovered());
    if (newHover) {
        setHovered(newHover);
    }

    OpenSim::Component const* newIsolated = relocateComponentPointerToAnotherModel(*m_Impl->m_Model, uim.getIsolated());
    if (newIsolated) {
        setIsolated(newIsolated);
    }
}

void osc::UiModel::pushCoordinateEdit(OpenSim::Coordinate const& c, CoordinateEdit const& ce) {
    m_Impl->m_StateModifications.pushCoordinateEdit(c, ce);
    m_Impl->m_State = initializeState(*m_Impl->m_Model, m_Impl->m_StateModifications);
    generateDecorations(*m_Impl->m_Model, *m_Impl->m_State, m_Impl->m_FixupScaleFactor, m_Impl->m_Decorations);
    updateBVH(m_Impl->m_Decorations, m_Impl->m_SceneBVH);
}

AABB osc::UiModel::getSceneAABB() const {
    auto const& bvh = getSceneBVH();
    if (!bvh.nodes.empty()) {
        return bvh.nodes[0].bounds;
    } else {
        return AABB{};
    }
}

glm::vec3 osc::UiModel::getSceneDimensions() const {
    return AABBDims(getSceneAABB());
}

float osc::UiModel::getSceneLongestDimension() const {
    return AABBLongestDim(getSceneAABB());
}

float osc::UiModel::getRecommendedScaleFactor() const {
    // generate decorations as if they were empty-sized and union their
    // AABBs to get an idea of what the "true" scale of the model probably
    // is (without the model containing oversized frames, etc.)
    std::vector<LabelledSceneElement> ses;
    generateDecorations(*m_Impl->m_Model, *m_Impl->m_State, 0.0f, ses);

    if (ses.empty()) {
        return 1.0f;
    }

    AABB aabb = ses[0].worldspaceAABB;
    for (size_t i = 1; i < ses.size(); ++i) {
        aabb = AABBUnion(aabb, ses[i].worldspaceAABB);
    }

    float longest = AABBLongestDim(aabb);
    float rv = 1.0f;

    while (longest < 0.1) {
        longest *= 10.0f;
        rv /= 10.0f;
    }

    return rv;
}

std::chrono::system_clock::time_point osc::UiModel::getLastModifiedTime() const {
    return m_Impl->m_LastModified;
}

// declare the death of a component pointer
//
// this happens when we know that OpenSim has destructed a component in
// the model indirectly (e.g. it was destructed by an OpenSim container)
// and that we want to ensure the pointer isn't still held by this state
void osc::UiModel::declareDeathOf(OpenSim::Component const* c) noexcept {
    if (getSelected() == c) {
        setSelected(nullptr);
    }

    if (getHovered() == c) {
        setHovered(nullptr);
    }

    if (getIsolated() == c) {
        setIsolated(nullptr);
    }
}

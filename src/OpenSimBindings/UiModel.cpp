#include "UiModel.hpp"

#include "src/3D/Model.hpp"
#include "src/OpenSimBindings/RenderableScene.hpp"
#include "src/SimTKBindings/SceneGeneratorNew.hpp"
#include "src/SimTKBindings/SimTKConverters.hpp"
#include "src/App.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PointToPointSpring.h>
#include <OpenSim/Common/ModelDisplayHints.h>
#include <SimTKcommon.h>

#include <algorithm>
#include <memory>
#include <vector>

using namespace osc;

// translate a pointer to a component in model A to a pointer to a component in model B
//
// returns nullptr if the pointer cannot be cleanly translated
static OpenSim::Component* relocateComponentPointerToAnotherModel(OpenSim::Model const& model, OpenSim::Component* ptr) {
    if (!ptr) {
        return nullptr;
    }

    try {
        return const_cast<OpenSim::Component*>(model.findComponent(ptr->getAbsolutePath()));
    } catch (OpenSim::Exception const&) {
        // finding fails with exception when ambiguous (fml)
        return nullptr;
    }
}

static void getSceneElements(OpenSim::Model const& m,
                             SimTK::State const& st,
                             float fixupScaleFactor,
                             std::vector<LabelledSceneElement>& out) {
    out.clear();

    OpenSim::Component const* currentComponent = nullptr;
    auto onEmit = [&](SceneElement const& se) {
        out.emplace_back(se, currentComponent);
    };

    SceneGeneratorLambda visitor{App::meshes(), m.getSystem().getMatterSubsystem(), st, fixupScaleFactor, onEmit};

    OpenSim::ModelDisplayHints mdh = m.getDisplayHints();
    mdh.set_show_frames(true);

    SimTK::Array_<SimTK::DecorativeGeometry> geomList;
    for (OpenSim::Component const& c : m.getComponentList()) {
        currentComponent = &c;

        // if the component is an OpenSim::PointToPointSpring, then hackily generate
        // a cylinder between the spring's two attachment points
        //
        // (these fixups should ideally be in a lookup table)
        if (typeid(c) == typeid(OpenSim::PointToPointSpring)) {
            auto const& p2p = static_cast<OpenSim::PointToPointSpring const&>(c);
            glm::mat4 b1LocalToGround = SimTKMat4x4FromTransform(p2p.getBody1().getTransformInGround(st));
            glm::mat4 b2LocalToGround =  SimTKMat4x4FromTransform(p2p.getBody2().getTransformInGround(st));
            glm::vec3 p1Local = SimTKVec3FromVec3(p2p.getPoint1());
            glm::vec3 p2Local = SimTKVec3FromVec3(p2p.getPoint2());

            // two points of the connecting cylinder
            glm::vec3 p1Ground = b1LocalToGround * glm::vec4{p1Local, 1.0f};
            glm::vec3 p2Ground = b2LocalToGround * glm::vec4{p2Local, 1.0f};
            glm::vec3 p1Cylinder = {0.0f, -1.0f, 0.0f};
            glm::vec3 p2Cylinder = {0.0f, +1.0f, 0.0f};
            Segment springLine{p1Ground, p2Ground};
            Segment cylinderLine{p1Cylinder, p2Cylinder};

            glm::mat4 cylinderXform = SegmentToSegmentXform(cylinderLine, springLine);
            glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, {0.005f * fixupScaleFactor, 1.0f, 0.005f * fixupScaleFactor});

            SceneElement se;
            se.mesh = App::cur().getMeshCache().getCylinderMesh();
            se.modelMtx = cylinderXform * scaler;
            se.normalMtx = NormalMatrix(se.modelMtx);
            se.color = {0.7f, 0.7f, 0.7f, 1.0f};
            se.worldspaceAABB = AABBApplyXform(se.mesh->getAABB(), se.modelMtx);

            onEmit(se);
        }

        // if the component is a geometry path that is owned by a muscle then coerce the selection
        // to the muscle, so that users see+select muscle components in the UI
        if (dynamic_cast<OpenSim::GeometryPath const*>(&c)) {
            if (c.hasOwner()) {
                if (auto const* musc = dynamic_cast<OpenSim::Muscle const*>(&c.getOwner()); musc) {
                    currentComponent = musc;
                }
            }
        }

        c.generateDecorations(true, mdh, st, geomList);
        for (SimTK::DecorativeGeometry const& dg : geomList) {
            dg.implementGeometry(visitor);
        }
        geomList.clear();

        c.generateDecorations(false, mdh, st, geomList);
        for (SimTK::DecorativeGeometry const& dg : geomList) {
            dg.implementGeometry(visitor);
        }
        geomList.clear();
    }
}

static bool sortByOpacityThenMeshID(SceneElement const& a, SceneElement const& b) {
    if (a.color.a != a.color.a) {
        return a.color.a < b.color.a;
    } else {
        return a.mesh.get() < b.mesh.get();
    }
}

static std::unique_ptr<SimTK::State> initializeState(OpenSim::Model& m, StateModifications& modifications) {
    std::unique_ptr<SimTK::State> rv = std::make_unique<SimTK::State>(m.initializeState());
    modifications.applyToState(m, *rv);
    m.equilibrateMuscles(*rv);
    m.realizePosition(*rv);
    return rv;
}

void osc::StateModifications::pushCoordinateEdit(const OpenSim::Coordinate& c, const CoordinateEdit& ce) {
    m_CoordEdits[c.getAbsolutePathString()] = ce;
}

bool osc::CoordinateEdit::applyToState(OpenSim::Coordinate const& c, SimTK::State& st) const {
    bool applied = false;

    bool wasLocked = c.getLocked(st);

    // always unlock to apply user edits
    if (wasLocked) {
        c.setLocked(st, false);
    }

    if (c.getValue(st) != value) {
        c.setValue(st, value);
        applied = true;
    }

    if (c.getSpeedValue(st) != speed) {
        c.setSpeedValue(st, speed);
        applied = true;
    }

    // apply the final lock state (was unconditionally unlocked, above)
    c.setLocked(st, locked);
    if (wasLocked != locked) {
        applied = true;
    }

    return applied;
}

bool osc::StateModifications::applyToState(const OpenSim::Model& m, SimTK::State& st) const {
    bool rv = false;

    for (auto& p : m_CoordEdits) {
        if (!m.hasComponent(p.first)) {
            continue;  // TODO: evict it
        }

        OpenSim::Coordinate const& c = m.getComponent<OpenSim::Coordinate>(p.first);

        bool modifiedCoord = p.second.applyToState(c, st);

        if (!modifiedCoord) {
            // TODO: evict it
        }

        rv = rv || modifiedCoord;
    }

    return rv;
}

void osc::generateDecorations(OpenSim::Model const& model,
                              SimTK::State const& state,
                              float fixupScaleFactor,
                              std::vector<LabelledSceneElement>& out) {
    out.clear();
    getSceneElements(model, state, fixupScaleFactor, out);
    std::sort(out.begin(), out.end(), sortByOpacityThenMeshID);
}

void osc::updateBVH(nonstd::span<LabelledSceneElement const> sceneEls, BVH& bvh) {
    std::vector<AABB> aabbs;
    aabbs.reserve(sceneEls.size());
    for (auto const& el : sceneEls) {
        aabbs.push_back(el.worldspaceAABB);
    }
    BVH_BuildFromAABBs(bvh, aabbs.data(), aabbs.size());
}


osc::UiModel::UiModel(std::string const& osim) :
    UiModel{std::make_unique<OpenSim::Model>(osim)} {
}

osc::UiModel::UiModel(std::unique_ptr<OpenSim::Model> _model) :
    stateModifications{},
    model{[&_model]() {
        _model->finalizeFromProperties();
        _model->finalizeConnections();
        _model->buildSystem();
        return std::unique_ptr<OpenSim::Model>{std::move(_model)};
    }()},
    state{initializeState(*model, stateModifications)},
    decorations{},
    sceneAABBBVH{},
    fixupScaleFactor{1.0f},
    selected{nullptr},
    hovered{nullptr},
    isolated{nullptr},
    timestamp{std::chrono::system_clock::now()} {

    generateDecorations(*model, *state, fixupScaleFactor, decorations);
    updateBVH(decorations, sceneAABBBVH);
}

osc::UiModel::UiModel(UiModel const& other, std::chrono::system_clock::time_point t) :
    stateModifications{other.stateModifications},
    model{[&other]() {
        auto copy = std::make_unique<OpenSim::Model>(*other.model);
        copy->finalizeFromProperties();
        copy->finalizeConnections();
        copy->buildSystem();
        return copy;
    }()},
    state{initializeState(*model, stateModifications)},
    decorations{},
    sceneAABBBVH{},
    fixupScaleFactor{other.fixupScaleFactor},
    selected{relocateComponentPointerToAnotherModel(*model, other.selected)},
    hovered{relocateComponentPointerToAnotherModel(*model, other.hovered)},
    isolated{relocateComponentPointerToAnotherModel(*model, other.isolated)},
    timestamp{t} {

    generateDecorations(*model, *state, fixupScaleFactor, decorations);
    updateBVH(decorations, sceneAABBBVH);
}

osc::UiModel::UiModel(UiModel const& other) :
    UiModel{other, std::chrono::system_clock::now()} {
}

osc::UiModel::UiModel(UiModel&&) noexcept = default;

osc::UiModel::~UiModel() noexcept = default;

osc::UiModel& osc::UiModel::operator=(UiModel&&) = default;

void osc::UiModel::onUiModelModified() {
    model->finalizeFromProperties();
    model->finalizeConnections();
    model->buildSystem();
    this->state = initializeState(*model, stateModifications);
    generateDecorations(*model, *state, fixupScaleFactor, decorations);
    updateBVH(decorations, sceneAABBBVH);
    this->timestamp = std::chrono::system_clock::now();
}

void osc::UiModel::pushCoordinateEdit(OpenSim::Coordinate const& c, CoordinateEdit const& ce) {
    stateModifications.pushCoordinateEdit(c, ce);
    this->state = initializeState(*model, stateModifications);
    generateDecorations(*model, *state, fixupScaleFactor, decorations);
    updateBVH(decorations, sceneAABBBVH);
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
    generateDecorations(*model, *state, 0.0f, ses);

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

void osc::UiModel::setSceneScaleFactor(float sf) {
    fixupScaleFactor = sf;
    generateDecorations(*model, *state, fixupScaleFactor, decorations);
    updateBVH(decorations, sceneAABBBVH);
}


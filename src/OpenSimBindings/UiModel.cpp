#include "UiModel.hpp"

#include "src/OpenSimBindings/RenderableScene.hpp"
#include "src/SimTKBindings/SceneGeneratorNew.hpp"
#include "src/App.hpp"

#include <OpenSim/Simulation/Model/Model.h>
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

static std::unique_ptr<SimTK::State> initializeState(OpenSim::Model& m, std::unordered_map<std::string, CoordinateEdit>& coordEdits) {
    std::unique_ptr<SimTK::State> rv = std::make_unique<SimTK::State>(m.initializeState());

    for (auto& p : coordEdits) {
        if (!m.hasComponent(p.first)) {
            continue;  // TODO: evict it
        }

        OpenSim::Coordinate const& c = m.getComponent<OpenSim::Coordinate>(p.first);

        bool applied = false;

        if (c.getValue(*rv) != p.second.value) {
            c.setValue(*rv, p.second.value);
            applied = true;
        }

        if (c.getLocked(*rv) != p.second.locked) {
            c.setLocked(*rv, p.second.locked);
            applied = true;
        }

        if (!applied) {
            // TODO: evict it
        }
    }
    m.equilibrateMuscles(*rv);
    m.realizePosition(*rv);
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
    coordEdits{},
    model{[&_model]() {
        _model->finalizeFromProperties();
        _model->finalizeConnections();
        _model->buildSystem();
        return std::unique_ptr<OpenSim::Model>{std::move(_model)};
    }()},
    state{initializeState(*model, coordEdits)},
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
    coordEdits{other.coordEdits},
    model{[&other]() {
        auto copy = std::make_unique<OpenSim::Model>(*other.model);
        copy->finalizeFromProperties();
        copy->finalizeConnections();
        copy->buildSystem();
        return copy;
    }()},
    state{initializeState(*model, coordEdits)},
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
    this->state = initializeState(*model, coordEdits);
    generateDecorations(*model, *state, fixupScaleFactor, decorations);
    updateBVH(decorations, sceneAABBBVH);
    this->timestamp = std::chrono::system_clock::now();
}

void osc::UiModel::addCoordinateEdit(OpenSim::Coordinate const& c, CoordinateEdit ce) {
    coordEdits[c.getAbsolutePathString()] = ce;
    this->state = initializeState(*model, coordEdits);
    generateDecorations(*model, *state, fixupScaleFactor, decorations);
    updateBVH(decorations, sceneAABBBVH);
}

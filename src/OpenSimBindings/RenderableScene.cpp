#include "RenderableScene.hpp"

#include "src/SimTKBindings/SceneGeneratorNew.hpp"
#include "src/SimTKBindings/SimTKConverters.hpp"
#include "src/App.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <OpenSim/Common/ModelDisplayHints.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PointToPointSpring.h>
#include <SimTKcommon.h>

#include <algorithm>
#include <vector>

using namespace osc;

static bool sortByOpacityThenMeshID(SceneElement const& a, SceneElement const& b) {
    if (a.color.a != b.color.a) {
        return a.color.a > b.color.a;  // alpha descending, so non-opaque stuff is drawn last
    } else {
        return a.mesh.get() < b.mesh.get();
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

    OpenSim::ModelDisplayHints const& mdh = m.getDisplayHints();

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

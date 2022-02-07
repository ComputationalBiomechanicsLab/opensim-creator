#include "RenderableScene.hpp"

#include "src/SimTKBindings/SceneGeneratorNew.hpp"
#include "src/SimTKBindings/SimTKConverters.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/App.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <OpenSim/Common/ModelDisplayHints.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PointToPointSpring.h>
#include <SimTKcommon.h>

#include <vector>

using namespace osc;


static bool HasGreaterAlphaOrLowerMeshID(SceneElement const& a, SceneElement const& b)
{
    if (a.color.a != b.color.a)
    {
        // alpha descending, so non-opaque stuff is drawn last
        return a.color.a > b.color.a;
    }
    else
    {
        return a.mesh.get() < b.mesh.get();
    }
}

static void HandlePointToPointSpring(float fixupScaleFactor,
                                     SimTK::State const& st,
                                     OpenSim::PointToPointSpring const& p2p,
                                     std::vector<LabelledSceneElement>& out)
{
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

    out.emplace_back(std::move(se), &p2p);
}

static void HandleStation(float fixupScaleFactor,
                          SimTK::State const& st,
                          OpenSim::Station const& s,
                          std::vector<LabelledSceneElement>& out)
{
    glm::vec3 loc = SimTKVec3FromVec3(s.getLocationInGround(st));
    float r = fixupScaleFactor * 0.005f;
    glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, {r, r, r});
    glm::mat4 translater = glm::translate(glm::mat4{1.0f}, loc);

    SceneElement se;
    se.mesh = App::cur().getMeshCache().getSphereMesh();
    se.modelMtx = translater * scaler;
    se.normalMtx = NormalMatrix(se.modelMtx);
    se.color = {1.0f, 0.0f, 0.0f, 1.0f};
    se.worldspaceAABB = AABBApplyXform(se.mesh->getAABB(), se.modelMtx);

    out.emplace_back(std::move(se), &s);
}

template<typename Callback>
static void HandleGenericOpenSimElement(OpenSim::Component const& c,
                                        SimTK::State const& st,
                                        OpenSim::ModelDisplayHints const& mdh,
                                        SimTK::Array_<SimTK::DecorativeGeometry>& geomList,
                                        SceneGeneratorLambda<Callback>& visitor)
{
    c.generateDecorations(true, mdh, st, geomList);
    for (SimTK::DecorativeGeometry const& dg : geomList)
    {
        dg.implementGeometry(visitor);
    }
    geomList.clear();

    c.generateDecorations(false, mdh, st, geomList);
    for (SimTK::DecorativeGeometry const& dg : geomList)
    {
        dg.implementGeometry(visitor);
    }
    geomList.clear();
}

static void getSceneElements(OpenSim::Model const& m,
                             SimTK::State const& st,
                             float fixupScaleFactor,
                             std::vector<LabelledSceneElement>& out)
{
    out.clear();

    OpenSim::Component const* currentComponent = nullptr;
    auto onEmit = [&](SceneElement const& se)
    {
        out.emplace_back(se, currentComponent);
    };

    SceneGeneratorLambda visitor{
        App::meshes(),
        m.getSystem().getMatterSubsystem(),
        st,
        fixupScaleFactor,
        onEmit
    };

    OpenSim::ModelDisplayHints const& mdh = m.getDisplayHints();
    SimTK::Array_<SimTK::DecorativeGeometry> geomList;

    for (OpenSim::Component const& c : m.getComponentList())
    {
        currentComponent = &c;

        if (typeid(c) == typeid(OpenSim::PointToPointSpring))
        {
            // PointToPointSpring has no decoration in OpenSim, emit a custom geometry for it
            auto const& p2p = static_cast<OpenSim::PointToPointSpring const&>(c);
            HandlePointToPointSpring(fixupScaleFactor, st, p2p, out);
        }
        else if (typeid(c) == typeid(OpenSim::Station))
        {
            // Station has no decoration in OpenSim, emit custom geometry for it
            OpenSim::Station const& s = dynamic_cast<OpenSim::Station const&>(c);
            HandleStation(fixupScaleFactor, st, s, out);
        }
        else if (dynamic_cast<OpenSim::GeometryPath const*>(&c))
        {
            // GeometryPath requires custom *selection* logic
            //
            // if it's owned by a muscle then hittesting should return a muscle
            if (c.hasOwner())
            {
                if (auto const* musc = dynamic_cast<OpenSim::Muscle const*>(&c.getOwner()); musc)
                {
                    currentComponent = musc;
                }
            }

            HandleGenericOpenSimElement(c, st, mdh, geomList, visitor);
        }
        else
        {
            HandleGenericOpenSimElement(c, st, mdh, geomList, visitor);
        }
    }
}

void osc::generateDecorations(OpenSim::Model const& model,
                              SimTK::State const& state,
                              float fixupScaleFactor,
                              std::vector<LabelledSceneElement>& out)
{
    out.clear();
    getSceneElements(model, state, fixupScaleFactor, out);
    Sort(out, HasGreaterAlphaOrLowerMeshID);
}

void osc::updateBVH(nonstd::span<LabelledSceneElement const> sceneEls, BVH& bvh)
{
    std::vector<AABB> aabbs;
    aabbs.reserve(sceneEls.size());

    for (auto const& el : sceneEls)
    {
        aabbs.push_back(el.worldspaceAABB);
    }

    BVH_BuildFromAABBs(bvh, aabbs.data(), aabbs.size());
}

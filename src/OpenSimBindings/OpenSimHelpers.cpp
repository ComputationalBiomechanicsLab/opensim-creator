#include "OpenSimHelpers.hpp"

#include "src/OpenSimBindings/ComponentDecoration.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/SimTKHelpers.hpp"
#include "src/App.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PointToPointSpring.h>
#include <Simbody.h>

static bool HasGreaterAlphaOrLowerMeshID(osc::SystemDecoration const& a, osc::SystemDecoration const& b)
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
                                     std::vector<osc::ComponentDecoration>& out)
{
    glm::mat4 b1LocalToGround = osc::SimTKMat4x4FromTransform(p2p.getBody1().getTransformInGround(st));
    glm::mat4 b2LocalToGround =  osc::SimTKMat4x4FromTransform(p2p.getBody2().getTransformInGround(st));
    glm::vec3 p1Local = osc::SimTKVec3FromVec3(p2p.getPoint1());
    glm::vec3 p2Local = osc::SimTKVec3FromVec3(p2p.getPoint2());

    // two points of the connecting cylinder
    glm::vec3 p1Ground = b1LocalToGround * glm::vec4{p1Local, 1.0f};
    glm::vec3 p2Ground = b2LocalToGround * glm::vec4{p2Local, 1.0f};
    glm::vec3 p1Cylinder = {0.0f, -1.0f, 0.0f};
    glm::vec3 p2Cylinder = {0.0f, +1.0f, 0.0f};
    osc::Segment springLine{p1Ground, p2Ground};
    osc::Segment cylinderLine{p1Cylinder, p2Cylinder};

    glm::mat4 cylinderXform = SegmentToSegmentXform(cylinderLine, springLine);
    glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, {0.005f * fixupScaleFactor, 1.0f, 0.005f * fixupScaleFactor});

    osc::SystemDecoration se;
    se.mesh = osc::App::cur().getMeshCache().getCylinderMesh();
    se.modelMtx = cylinderXform * scaler;
    se.normalMtx = osc::NormalMatrix(se.modelMtx);
    se.color = {0.7f, 0.7f, 0.7f, 1.0f};
    se.worldspaceAABB = AABBApplyXform(se.mesh->getAABB(), se.modelMtx);

    out.emplace_back(std::move(se), &p2p);
}

static void HandleStation(float fixupScaleFactor,
                          SimTK::State const& st,
                          OpenSim::Station const& s,
                          std::vector<osc::ComponentDecoration>& out)
{
    glm::vec3 loc = osc::SimTKVec3FromVec3(s.getLocationInGround(st));
    float r = fixupScaleFactor * 0.005f;
    glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, {r, r, r});
    glm::mat4 translater = glm::translate(glm::mat4{1.0f}, loc);

    osc::SystemDecoration se;
    se.mesh = osc::App::cur().getMeshCache().getSphereMesh();
    se.modelMtx = translater * scaler;
    se.normalMtx = osc::NormalMatrix(se.modelMtx);
    se.color = {1.0f, 0.0f, 0.0f, 1.0f};
    se.worldspaceAABB = AABBApplyXform(se.mesh->getAABB(), se.modelMtx);

    out.emplace_back(std::move(se), &s);
}

static void HandleGenericOpenSimElement(OpenSim::Component const& c,
                                        SimTK::State const& st,
                                        OpenSim::ModelDisplayHints const& mdh,
                                        SimTK::Array_<SimTK::DecorativeGeometry>& geomList,
                                        osc::DecorativeGeometryHandler& handler)
{
    c.generateDecorations(true, mdh, st, geomList);
    for (SimTK::DecorativeGeometry const& dg : geomList)
    {
        handler(dg);
    }
    geomList.clear();

    c.generateDecorations(false, mdh, st, geomList);
    for (SimTK::DecorativeGeometry const& dg : geomList)
    {
        handler(dg);
    }
    geomList.clear();
}

static void getSceneElements(OpenSim::Model const& m,
                             SimTK::State const& st,
                             float fixupScaleFactor,
                             std::vector<osc::ComponentDecoration>& out)
{
    out.clear();

    OpenSim::Component const* currentComponent = nullptr;

    std::function<void(osc::SystemDecorationNew const&)> onEmit{[&currentComponent, &out](osc::SystemDecorationNew const& sd)
    {
        out.emplace_back(sd, currentComponent);
    }};

    osc::DecorativeGeometryHandler handler{
        osc::App::meshes(),
        m.getSystem().getMatterSubsystem(),
        st,
        fixupScaleFactor,
        onEmit
    };

    OpenSim::ModelDisplayHints const& mdh = m.getDisplayHints();
    SimTK::Array_<SimTK::DecorativeGeometry> geomList;

    for (OpenSim::Component const& c : m.getComponentList())
    {
        if (!osc::ShouldShowInUI(c))
        {
            continue;
        }

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

            HandleGenericOpenSimElement(c, st, mdh, geomList, handler);
        }
        else
        {
            HandleGenericOpenSimElement(c, st, mdh, geomList, handler);
        }
    }
}


// public API

std::vector<OpenSim::AbstractSocket const*> osc::GetAllSockets(OpenSim::Component& c)
{
    std::vector<OpenSim::AbstractSocket const*> rv;

    for (std::string const& name : c.getSocketNames())
    {
        OpenSim::AbstractSocket const& sock = c.getSocket(name);
        rv.push_back(&sock);
    }

    return rv;
}

std::vector<OpenSim::AbstractSocket const*> osc::GetSocketsWithTypeName(OpenSim::Component& c, std::string_view typeName)
{
    std::vector<OpenSim::AbstractSocket const*> rv;

    for (std::string const& name : c.getSocketNames())
    {
        OpenSim::AbstractSocket const& sock = c.getSocket(name);
        if (sock.getConnecteeTypeName() == typeName)
        {
            rv.push_back(&sock);
        }
    }

    return rv;
}

std::vector<OpenSim::AbstractSocket const*> osc::GetPhysicalFrameSockets(OpenSim::Component& c)
{
    return GetSocketsWithTypeName(c, "PhysicalFrame");
}

OpenSim::Component const* osc::FindComponent(OpenSim::Component const& c, OpenSim::ComponentPath const& cp)
{
    static OpenSim::ComponentPath const g_EmptyPath;

    if (cp == g_EmptyPath)
    {
        return nullptr;
    }

    try
    {
        return &c.getComponent(cp);
    }
    catch (OpenSim::Exception const&)
    {
        return nullptr;
    }
}

OpenSim::Component* osc::FindComponentMut(OpenSim::Component& c, OpenSim::ComponentPath const& cp)
{
    return const_cast<OpenSim::Component*>(FindComponent(c, cp));
}

bool osc::ContainsComponent(OpenSim::Component const& root, OpenSim::ComponentPath const& cp)
{
    return FindComponent(root, cp);
}

bool osc::HasInputFileName(OpenSim::Model const& m)
{
    std::string const& name = m.getInputFileName();
    return !name.empty() && name != "Unassigned";
}

std::filesystem::path osc::TryFindInputFile(OpenSim::Model const& m)
{
    if (!HasInputFileName(m))
    {
        return {};
    }

    std::filesystem::path p{m.getInputFileName()};

    if (!std::filesystem::exists(p))
    {
        return {};
    }

    return p;
}

bool osc::ShouldShowInUI(OpenSim::Component const& c)
{
    if (Is<OpenSim::PathWrapPoint>(c))
    {
        return false;
    }
    else if (Is<OpenSim::Station>(c) && c.hasOwner() && DerivesFrom<OpenSim::PathPoint>(c.getOwner()))
    {
        return false;
    }
    else
    {
        return true;
    }
}

void osc::GenerateModelDecorations(OpenSim::Model const& model,
                              SimTK::State const& state,
                              float fixupScaleFactor,
                              std::vector<ComponentDecoration>& out)
{
    out.clear();
    getSceneElements(model, state, fixupScaleFactor, out);
    Sort(out, HasGreaterAlphaOrLowerMeshID);
}

void osc::UpdateSceneBVH(nonstd::span<ComponentDecoration const> sceneEls, BVH& bvh)
{
    std::vector<AABB> aabbs;
    aabbs.reserve(sceneEls.size());

    for (auto const& el : sceneEls)
    {
        aabbs.push_back(el.worldspaceAABB);
    }

    BVH_BuildFromAABBs(bvh, aabbs.data(), aabbs.size());
}

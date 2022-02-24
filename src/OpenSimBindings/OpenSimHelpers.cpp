#include "OpenSimHelpers.hpp"

#include "src/OpenSimBindings/ComponentDecoration.hpp"
#include "src/3D/Model.hpp"
#include "src/MeshCache.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/Perf.hpp"
#include "src/Utils/SimTKHelpers.hpp"
#include "src/App.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PointToPointSpring.h>
#include <Simbody.h>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include <vector>

using namespace osc;


static bool HasGreaterAlphaOrLowerMeshID(ComponentDecoration const& a,
                                         ComponentDecoration const& b)
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

static Transform TransformInGround(OpenSim::PhysicalFrame const& pf, SimTK::State const& st)
{
    return ToTransform(pf.getTransformInGround(st));
}

static void HandlePointToPointSpring(float fixupScaleFactor,
                                     SimTK::State const& st,
                                     OpenSim::PointToPointSpring const& p2p,
                                     std::vector<osc::ComponentDecoration>& out)
{
    glm::vec3 p1 = transformPoint(TransformInGround(p2p.getBody1(), st), ToVec3(p2p.getPoint1()));
    glm::vec3 p2 = transformPoint(TransformInGround(p2p.getBody2(), st), ToVec3(p2p.getPoint2()));

    float radius = 0.005f * fixupScaleFactor;
    Transform cylinderXform = SimbodyCylinderToSegmentTransform({p1, p2}, radius);

    out.emplace_back(
        App::meshes().getCylinderMesh(),
        cylinderXform,
        glm::vec4{0.7f, 0.7f, 0.7f, 1.0f},
        &p2p
    );
}

static void HandleStation(float fixupScaleFactor,
                          SimTK::State const& st,
                          OpenSim::Station const& s,
                          std::vector<osc::ComponentDecoration>& out)
{
    float radius = fixupScaleFactor * 0.005f;

    Transform xform;
    xform.position = ToVec3(s.getLocationInGround(st));
    xform.scale = {radius, radius, radius};

    out.emplace_back(
        App::meshes().getSphereMesh(),
        xform,
        glm::vec4{1.0f, 0.0f, 0.0f, 1.0f},
        &s
    );
}

static void HandleGenericOpenSimElement(OpenSim::Component const& c,
                                        SimTK::State const& st,
                                        OpenSim::ModelDisplayHints const& mdh,
                                        SimTK::Array_<SimTK::DecorativeGeometry>& geomList,
                                        osc::DecorationProducer& handler)
{
    {
        OSC_PERF("OpenSim::Component::generateDecorations(true, ...)");
        c.generateDecorations(true, mdh, st, geomList);
    }

    {
        OSC_PERF("(pump fixed decorations into OSC)");
        for (SimTK::DecorativeGeometry const& dg : geomList)
        {
            handler(dg);
        }
    }
    geomList.clear();

    {
        OSC_PERF("OpenSim::Component::generateDecorations(false, ...)");
        c.generateDecorations(false, mdh, st, geomList);
    }

    {
        OSC_PERF("(pump dynamic decorations into OSC)");
        for (SimTK::DecorativeGeometry const& dg : geomList)
        {
            handler(dg);
        }
    }
    geomList.clear();
}

namespace
{
    // used whenever the SimTK backend emits something
    class OpenSimDecorationConsumer final : public DecorationConsumer {
    public:
        OpenSimDecorationConsumer(std::vector<ComponentDecoration>* out,
                                  OpenSim::Component const** currentComponent) :
            m_Out{std::move(out)},
            m_CurrentComponent{std::move(currentComponent)}
        {
        }

        void operator()(std::shared_ptr<Mesh> const& mesh, Transform const& t, glm::vec4 const& color) override
        {
            m_Out->emplace_back(mesh, t, color, *m_CurrentComponent);
        }

    private:
        std::vector<ComponentDecoration>* m_Out;
        OpenSim::Component const** m_CurrentComponent;
    };
}

static void GenerateDecorationEls(OpenSim::Model const& m,
                                  SimTK::State const& st,
                                  float fixupScaleFactor,
                                  std::vector<osc::ComponentDecoration>& out,
                                  OpenSim::Component const*,
                                  OpenSim::Component const*)
{
    out.clear();

    OpenSim::Component const* currentComponent = nullptr;

    OpenSimDecorationConsumer consumer{&out, &currentComponent};

    MeshCache& meshCache = App::meshes();

    DecorationProducer producer{
        meshCache,
        m.getSystem().getMatterSubsystem(),
        st,
        fixupScaleFactor,
        consumer
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

            HandleGenericOpenSimElement(c, st, mdh, geomList, producer);
        }
        else
        {
            HandleGenericOpenSimElement(c, st, mdh, geomList, producer);
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
                              std::vector<ComponentDecoration>& out,
                              OpenSim::Component const* hovered,
                              OpenSim::Component const* selected)
{
    out.clear();

    {
        OSC_PERF("scene generation");
        GenerateDecorationEls(model, state, fixupScaleFactor, out, hovered, selected);
    }

    {
        OSC_PERF("scene sorting");
        Sort(out, HasGreaterAlphaOrLowerMeshID);
    }
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

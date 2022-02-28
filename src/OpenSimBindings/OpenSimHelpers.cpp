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

// geometry rendering/handling support
namespace
{
    // generic handler for any `OpenSim::Component`
    void HandleComponent(OpenSim::Component const& c,
                         SimTK::State const& st,
                         OpenSim::ModelDisplayHints const& mdh,
                         SimTK::Array_<SimTK::DecorativeGeometry>& geomList,
                         osc::DecorativeGeometryHandler& handler)
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

    // OSC-specific decoration handler for `OpenSim::PointToPointSpring`
    void HandlePointToPointSpring(OpenSim::PointToPointSpring const& p2p,
                                  SimTK::State const& st,
                                  float fixupScaleFactor,
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

    // OSC-specific decoration handler for `OpenSim::Station`
    void HandleStation(OpenSim::Station const& s,
                       SimTK::State const& st,
                       float fixupScaleFactor,
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

    // OSC-specific decoration handler for `OpenSim::Body`
    void HandleBody(OpenSim::Body const& b,
                    SimTK::State const& st,
                    float fixupScaleFactor,
                    OpenSim::Component const*,
                    OpenSim::Component const* hovered,
                    std::vector<osc::ComponentDecoration>& out,
                    OpenSim::ModelDisplayHints const& mdh,
                    SimTK::Array_<SimTK::DecorativeGeometry>& geomList,
                    DecorativeGeometryHandler& producer)
    {
        // bodies are drawn normally but *also* draw a center-of-mass sphere if they are
        // currently hovered
        if (&b == hovered && b.getMassCenter() != SimTK::Vec3{0.0, 0.0, 0.0})
        {
            float radius = fixupScaleFactor * 0.005f;
            Transform t = TransformInGround(b, st);
            t.position = transformPoint(t, ToVec3(b.getMassCenter()));
            t.scale = {radius, radius, radius};

            out.emplace_back(
                App::meshes().getSphereMesh(),
                t,
                glm::vec4{0.0f, 0.0f, 0.0f, 1.0f},
                &b
            );
        }

        HandleComponent(b, st, mdh, geomList, producer);
    }

    // OSC-specific decoration handler for `OpenSim::GeometryPath`
    void HandleGeometryPath(OpenSim::GeometryPath const& gp,
                            SimTK::State const& st,
                            OpenSim::Component const** currentComponent,
                            OpenSim::ModelDisplayHints const& mdh,
                            SimTK::Array_<SimTK::DecorativeGeometry>& geomList,
                            DecorativeGeometryHandler& producer)
    {
        // GeometryPath requires custom *selection* logic
        //
        // if it's owned by a muscle then hittesting should return a muscle
        if (gp.hasOwner())
        {
            if (auto const* musc = dynamic_cast<OpenSim::Muscle const*>(&gp.getOwner()); musc)
            {
                *currentComponent = musc;
            }
        }

        HandleComponent(gp, st, mdh, geomList, producer);
    }

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

    void GenerateDecorationEls(OpenSim::Model const& m,
                               SimTK::State const& st,
                               float fixupScaleFactor,
                               std::vector<osc::ComponentDecoration>& out,
                               OpenSim::Component const* selected,
                               OpenSim::Component const* hovered)
    {
        out.clear();

        OpenSim::Component const* currentComponent = nullptr;

        OpenSimDecorationConsumer consumer{&out, &currentComponent};

        MeshCache& meshCache = App::meshes();

        DecorativeGeometryHandler producer{
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

            // handle OSC-specific decoration specializations, or fallback to generic
            // component decoration handling
            if (auto const* p2p = dynamic_cast<OpenSim::PointToPointSpring const*>(&c))
            {
                HandlePointToPointSpring(*p2p, st, fixupScaleFactor, out);
            }
            else if (typeid(c) == typeid(OpenSim::Station))
            {
                HandleStation(static_cast<OpenSim::Station const&>(c), st, fixupScaleFactor, out);
            }
            else if (auto const* body = dynamic_cast<OpenSim::Body const*>(&c))
            {
                HandleBody(*body, st, fixupScaleFactor, selected, hovered, out, mdh, geomList, producer);
            }
            else if (auto const* gp = dynamic_cast<OpenSim::GeometryPath const*>(&c))
            {
                HandleGeometryPath(*gp, st, &currentComponent, mdh, geomList, producer);
            }
            else
            {
                HandleComponent(c, st, mdh, geomList, producer);
            }
        }
    }
}


// public API

void osc::GetCoordinatesInModel(OpenSim::Model const& m,
                                std::vector<OpenSim::Coordinate const*>& out)
{
    OpenSim::CoordinateSet const& s = m.getCoordinateSet();
    int len = s.getSize();
    out.reserve(out.size() + static_cast<size_t>(len));

    for (int i = 0; i < len; ++i)
    {
        out.push_back(&s[i]);
    }
}

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
                                   OpenSim::Component const* selected,
                                   OpenSim::Component const* hovered)
{
    out.clear();

    {
        OSC_PERF("scene generation");
        GenerateDecorationEls(model, state, fixupScaleFactor, out, selected, hovered);
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

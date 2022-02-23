#include "SimTKHelpers.hpp"

#include "src/3D/Model.hpp"
#include "src/Log.hpp"
#include "src/MeshCache.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <simbody/internal/SimbodyMatterSubsystem.h>
#include <SimTKcommon.h>

#include <memory>
#include <utility>

using namespace osc;

static inline constexpr float g_LineThickness = 0.005f;
static inline constexpr float g_FrameAxisLengthRescale = 0.25f;
static inline constexpr float g_FrameAxisThickness = 0.0025f;
static inline constexpr float g_ConeHeadLength = 0.2f;

// extracts scale factors from geometry
static glm::vec3 GetScaleFactors(SimTK::DecorativeGeometry const& geom)
{
    SimTK::Vec3 sf = geom.getScaleFactors();
    for (int i = 0; i < 3; ++i)
    {
        sf[i] = sf[i] <= 0 ? 1.0 : sf[i];
    }
    return glm::vec3{sf[0], sf[1], sf[2]};
}

// returns a SimTK::Rotation re-expressed as a quaternion
static glm::quat RotationAsQuat(SimTK::Rotation const& r)
{
    SimTK::Quaternion q = r.convertRotationToQuaternion();
    glm::quat rv = {static_cast<float>(q[0]), static_cast<float>(q[1]), static_cast<float>(q[2]), static_cast<float>(q[3])};
    return glm::normalize(rv);  // just to be sure
}

// returns the rotational part of the transform as a quaternion
static glm::quat RotationAsQuat(SimTK::Transform const& t)
{
    return RotationAsQuat(t.R());
}

// returns the positional part of the transform as a vec3
static glm::vec3 PositionAsVec3(SimTK::Transform const& t)
{
    return SimTKVec3FromVec3(t.p());
}

// returns an osc::Transform equivalent of a SimTK::Transform
static Transform ToOscTransform(SimTK::Transform const& t)
{
    return Transform{PositionAsVec3(t), RotationAsQuat(t)};
}

// extracts RGBA from geometry
static glm::vec4 GetColor(SimTK::DecorativeGeometry const& geom)
{
    SimTK::Vec3 const& rgb = geom.getColor();
    SimTK::Real ar = geom.getOpacity();
    ar = ar < 0.0 ? 1.0 : ar;
    return glm::vec4(rgb[0], rgb[1], rgb[2], ar);
}

// creates a geometry-to-ground transform for the given geometry
static Transform ToOscTransform(SimTK::SimbodyMatterSubsystem const& matter,
                                SimTK::State const& state,
                                SimTK::DecorativeGeometry const& g)
{
    SimTK::MobilizedBody const& mobod = matter.getMobilizedBody(SimTK::MobilizedBodyIndex(g.getBodyId()));
    SimTK::Transform body2ground = mobod.getBodyTransform(state);
    SimTK::Transform decoration2body = g.getTransform();
    SimTK::Transform decoration2ground = body2ground * decoration2body;

    Transform rv = ToOscTransform(decoration2ground);
    rv.scale = GetScaleFactors(g);

    return rv;
}

static Transform SegmentToSegmentTransform(Segment const& a, Segment const& b)
{
    glm::vec3 aLine = a.p2 - a.p1;
    glm::vec3 bLine = b.p2 - b.p1;

    float aLen = glm::length(aLine);
    float bLen = glm::length(bLine);

    glm::vec3 aDir = aLine/aLen;
    glm::vec3 bDir = bLine/bLen;

    glm::vec3 aMid = (a.p1 + a.p2)/2.0f;
    glm::vec3 bMid = (b.p1 + b.p2)/2.0f;

    // for scale: LERP [0,1] onto [1,l] along original direction
    Transform t;
    t.rotation = glm::rotation(aDir, bDir);
    t.scale = glm::vec3{1.0f, 1.0f, 1.0f} + (bLen/aLen-1.0f)*aDir;
    t.position = bMid - aMid;
    return t;
}

static Transform CylinderToLineTransform(glm::vec3 const& a, glm::vec3 const& b)
{
    Segment meshLine{{0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    Segment outputLine{a, b};
    return SegmentToSegmentTransform(meshLine, outputLine);
}

// get the `vert`th vertex of the `face`th face
static glm::vec3 GetFaceVertex(SimTK::PolygonalMesh const& mesh, int face, int vert)
{
    int vertidx = mesh.getFaceVertex(face, vert);
    SimTK::Vec3 const& data = mesh.getVertexPosition(vertidx);
    return SimTKVec3FromVec3(data);
}

// an implementation of SimTK::DecorativeGeometryImplementation that emits generic
// triangle-mesh-based SystemDecorations that can be consumed by the rest of the UI
class osc::DecorativeGeometryHandler::Impl final : public SimTK::DecorativeGeometryImplementation {
public:
    Impl(MeshCache& meshCache,
         SimTK::SimbodyMatterSubsystem const& matter,
         SimTK::State const& st,
         float fixupScaleFactor,
         std::function<void(SystemDecorationNew const&)>& callback) :

        m_MeshCache{meshCache},
        m_Matter{matter},
        m_St{st},
        m_FixupScaleFactor{fixupScaleFactor},
        m_Callback{callback}
    {
    }

private:
    Transform ToOscTransform(SimTK::DecorativeGeometry const& d) const
    {
        return ::ToOscTransform(m_Matter, m_St, d);
    }

    void implementPointGeometry(SimTK::DecorativePoint const&) override
    {
        static bool shown_once = []()
        {
            log::warn("this model uses implementPointGeometry, which is not yet implemented in OSC");
            return true;
        }();
        (void)shown_once;
    }

    void implementLineGeometry(SimTK::DecorativeLine const& d) override
    {
        Transform t = ToOscTransform(d);

        glm::vec3 p1 = transformPoint(t, SimTKVec4FromVec3(d.getPoint1()));
        glm::vec3 p2 = transformPoint(t, SimTKVec4FromVec3(d.getPoint2()));

        Transform cylinderXform = CylinderToLineTransform(p1, p2);
        cylinderXform.scale.x *= g_LineThickness * m_FixupScaleFactor;
        cylinderXform.scale.z *= g_LineThickness * m_FixupScaleFactor;
        cylinderXform.scale *= t.scale;

        m_Callback({m_MeshCache.getCylinderMesh(), cylinderXform, GetColor(d)});
    }

    void implementBrickGeometry(SimTK::DecorativeBrick const& d) override
    {
        Transform t = ToOscTransform(d);
        t.scale *= SimTKVec3FromVec3(d.getHalfLengths());

        m_Callback({m_MeshCache.getBrickMesh(), t, GetColor(d)});
    }

    void implementCylinderGeometry(SimTK::DecorativeCylinder const& d) override
    {
        float radius = static_cast<float>(d.getRadius());

        Transform t = ToOscTransform(d);
        t.scale.x *= radius;
        t.scale.y *= static_cast<float>(d.getHalfHeight());
        t.scale.z *= radius;

        m_Callback({m_MeshCache.getCylinderMesh(), t, GetColor(d)});
    }

    void implementCircleGeometry(SimTK::DecorativeCircle const&) override
    {
        static bool shownWarning = []()
        {
            log::warn("this model uses implementCircleGeometry, which is not yet implemented in OSC");
            return true;
        }();
        (void)shownWarning;
    }

    void implementSphereGeometry(SimTK::DecorativeSphere const& d) override
    {
        Transform t = ToOscTransform(d);
        t.scale *= m_FixupScaleFactor * static_cast<float>(d.getRadius());

        m_Callback({m_MeshCache.getSphereMesh(), t, GetColor(d)});
    }

    void implementEllipsoidGeometry(SimTK::DecorativeEllipsoid const& d) override
    {
        Transform t = ToOscTransform(d);
        t.scale *= SimTKVec3FromVec3(d.getRadii());

        m_Callback({m_MeshCache.getSphereMesh(), t, GetColor(d)});
    }

    void implementFrameGeometry(SimTK::DecorativeFrame const& d) override
    {
        Transform t = ToOscTransform(d);

        // emit origin sphere
        {
            float r = 0.05f * g_FrameAxisLengthRescale * m_FixupScaleFactor;
            Transform sphereXform = t.withScale(r);
            glm::vec4 white = {1.0f, 1.0f, 1.0f, 1.0f};

            m_Callback({m_MeshCache.getSphereMesh(), sphereXform, white});
        }

        // emit leg cylinders
        glm::vec3 axisLengths = t.scale * static_cast<float>(d.getAxisLength());
        for (int axis = 0; axis < 3; ++axis)
        {
            glm::vec3 dir = {0.0f, 0.0f, 0.0f};
            dir[axis] = g_FrameAxisLengthRescale * m_FixupScaleFactor * axisLengths[axis];

            Transform legXform = CylinderToLineTransform(t.position, t.position + transformDirection(t, dir));
            legXform.scale.x *= g_FrameAxisThickness * m_FixupScaleFactor;
            legXform.scale.z *= g_FrameAxisThickness * m_FixupScaleFactor;

            glm::vec4 color = {0.0f, 0.0f, 0.0f, 1.0f};
            color[axis] = 1.0f;

            m_Callback({m_MeshCache.getCylinderMesh(), legXform, color});
        }
    }

    void implementTextGeometry(SimTK::DecorativeText const&) override
    {
        static bool shownWarning = []()
        {
            log::warn("this model uses implementTextGeometry, which is not yet implemented in OSC");
            return true;
        }();
        (void)shownWarning;
    }

    void implementMeshGeometry(SimTK::DecorativeMesh const&) override
    {
        static bool shownWarning = []()
        {
            log::warn("this model uses implementMeshGeometry, which is not yet implemented in OSC");
            return true;
        }();
        (void)shownWarning;
    }

    void implementMeshFileGeometry(SimTK::DecorativeMeshFile const& d) override
    {
        m_Callback({m_MeshCache.getMeshFile(d.getMeshFile()), ToOscTransform(d), GetColor(d)});
    }

    void implementArrowGeometry(SimTK::DecorativeArrow const& d) override
    {
        Transform t = ToOscTransform(d);

        glm::vec3 startBase = SimTKVec3FromVec3(d.getStartPoint());
        glm::vec3 endBase = SimTKVec3FromVec3(d.getEndPoint());

        glm::vec3 start = transformPoint(t, startBase);
        glm::vec3 end = transformPoint(t, endBase);

        glm::vec3 dir = glm::normalize(end - start);

        glm::vec3 neckStart = start;
        glm::vec3 neckEnd = end - (static_cast<float>(d.getTipLength()) * dir);
        glm::vec3 const& headStart = neckEnd;
        glm::vec3 const& headEnd = end;

        constexpr float neckThickness = 0.005f;
        constexpr float headThickness = 0.02f;

        glm::vec4 color = GetColor(d);

        // emit neck cylinder
        {
            Transform neckXform = CylinderToLineTransform(neckStart, neckEnd);
            neckXform.scale.x *= neckThickness;
            neckXform.scale.z *= neckThickness;

            m_Callback({m_MeshCache.getCylinderMesh(), neckXform, color});
        }

        // emit head cone
        {
            Transform headXform = CylinderToLineTransform(headStart, headEnd);
            headXform.scale.x *= headThickness;
            headXform.scale.z *= headThickness;

            m_Callback({m_MeshCache.getConeMesh(), headXform, color});
        }
    }

    void implementTorusGeometry(SimTK::DecorativeTorus const&) override
    {
        static bool shownWarning = []() {
            log::warn("this model uses implementTorusGeometry, which is not yet implemented in OSC");
            return true;
        }();
        (void)shownWarning;
    }

    void implementConeGeometry(SimTK::DecorativeCone const& d) override
    {
        Transform t = ToOscTransform(d);

        glm::vec3 posBase = SimTKVec3FromVec3(d.getOrigin());
        glm::vec3 posDir = SimTKVec3FromVec3(d.getDirection());

        glm::vec3 pos = transformPoint(t, posBase);
        glm::vec3 dir = transformDirection(t, posDir);

        float radius = static_cast<float>(d.getBaseRadius());
        float height = static_cast<float>(d.getHeight());

        Transform coneXform = CylinderToLineTransform(pos, pos + height*dir);
        coneXform.scale.x *= radius * t.scale.x;
        coneXform.scale.z *= radius * t.scale.z;

        m_Callback({m_MeshCache.getConeMesh(), coneXform, GetColor(d)});
    }

    MeshCache& m_MeshCache;
    SimTK::SimbodyMatterSubsystem const& m_Matter;
    SimTK::State const& m_St;
    float m_FixupScaleFactor;
    std::function<void(SystemDecorationNew const&)>& m_Callback;
};


// public API

SimTK::Vec3 osc::SimTKVec3FromV3(float v[3])
{
    return
    {
        static_cast<double>(v[0]),
        static_cast<double>(v[1]),
        static_cast<double>(v[2]),
    };
}

SimTK::Vec3 osc::SimTKVec3FromV3(glm::vec3 const& v)
{
    return
    {
        static_cast<double>(v.x),
        static_cast<double>(v.y),
        static_cast<double>(v.z),
    };
}

SimTK::Inertia osc::SimTKInertiaFromV3(float v[3])
{
    return
    {
        static_cast<double>(v[0]),
        static_cast<double>(v[1]),
        static_cast<double>(v[2]),
    };
}

glm::vec3 osc::SimTKVec3FromVec3(SimTK::Vec3 const& v)
{
    return glm::vec3(v[0], v[1], v[2]);
}

glm::vec4 osc::SimTKVec4FromVec3(SimTK::Vec3 const& v, float w)
{
    return glm::vec4{v[0], v[1], v[2], w};
}

glm::mat4x3 osc::SimTKMat4x3FromXForm(SimTK::Transform const& t)
{
    // glm::mat4x3 is column major:
    //     see: https://glm.g-truc.net/0.9.2/api/a00001.html
    //     (and just Google "glm column major?")
    //
    // SimTK is row-major, carefully read the sourcecode for
    // `SimTK::Transform`.

    glm::mat4x3 m;

    // x0 y0 z0 w0
    SimTK::Rotation const& r = t.R();
    SimTK::Vec3 const& p = t.p();

    {
        auto const& row0 = r[0];
        m[0][0] = static_cast<float>(row0[0]);
        m[1][0] = static_cast<float>(row0[1]);
        m[2][0] = static_cast<float>(row0[2]);
        m[3][0] = static_cast<float>(p[0]);
    }

    {
        auto const& row1 = r[1];
        m[0][1] = static_cast<float>(row1[0]);
        m[1][1] = static_cast<float>(row1[1]);
        m[2][1] = static_cast<float>(row1[2]);
        m[3][1] = static_cast<float>(p[1]);
    }

    {
        auto const& row2 = r[2];
        m[0][2] = static_cast<float>(row2[0]);
        m[1][2] = static_cast<float>(row2[1]);
        m[2][2] = static_cast<float>(row2[2]);
        m[3][2] = static_cast<float>(p[2]);
    }

    return m;
}

glm::mat4x4 osc::SimTKMat4x4FromTransform(SimTK::Transform const& t)
{
    return glm::mat4{SimTKMat4x3FromXForm(t)};
}

SimTK::Transform osc::SimTKTransformFromMat4x3(glm::mat4x3 const& m)
{
    // glm::mat4 is column-major, SimTK::Transform is effectively
    // row-major

    SimTK::Mat33 mtx(
        static_cast<double>(m[0][0]), static_cast<double>(m[1][0]), static_cast<double>(m[2][0]),
        static_cast<double>(m[0][1]), static_cast<double>(m[1][1]), static_cast<double>(m[2][1]),
        static_cast<double>(m[0][2]), static_cast<double>(m[1][2]), static_cast<double>(m[2][2])
    );
    SimTK::Vec3 translation{m[3][0], m[3][1], m[3][2]};

    SimTK::Rotation rot{mtx};

    return SimTK::Transform{rot, translation};
}

Mesh osc::SimTKLoadMesh(std::filesystem::path const& p)
{
    SimTK::DecorativeMeshFile dmf{p.string()};
    SimTK::PolygonalMesh const& mesh = dmf.getMesh();

    MeshData rv;
    rv.reserve(static_cast<size_t>(mesh.getNumVertices()));

    uint32_t index = 0;
    auto push = [&rv, &index](glm::vec3 const& pos, glm::vec3 const& normal)
    {
        rv.verts.push_back(pos);
        rv.normals.push_back(normal);
        rv.indices.push_back(index++);
    };

    for (int face = 0, nfaces = mesh.getNumFaces(); face < nfaces; ++face)
    {
        int verts = mesh.getNumVerticesForFace(face);

        if (verts < 3)
        {
            // line/point

            // ignore

        }
        else if (verts == 3)
        {
            // triangle

            glm::vec3 vs[] =
            {
                GetFaceVertex(mesh, face, 0),
                GetFaceVertex(mesh, face, 1),
                GetFaceVertex(mesh, face, 2),
            };
            glm::vec3 normal = TriangleNormal(vs);

            push(vs[0], normal);
            push(vs[1], normal);
            push(vs[2], normal);

        }
        else if (verts == 4)
        {
            // quad: render as two triangles

            glm::vec3 vs[] =
            {
                GetFaceVertex(mesh, face, 0),
                GetFaceVertex(mesh, face, 1),
                GetFaceVertex(mesh, face, 2),
                GetFaceVertex(mesh, face, 3),
            };

            glm::vec3 norms[] =
            {
                TriangleNormal(vs[0], vs[1], vs[2]),
                TriangleNormal(vs[2], vs[3], vs[0]),
            };

            push(vs[0], norms[0]);
            push(vs[1], norms[0]);
            push(vs[2], norms[0]);
            push(vs[2], norms[1]);
            push(vs[3], norms[1]);
            push(vs[0], norms[1]);

        }
        else
        {
            // polygon (>3 edges):
            //
            // create a vertex at the average center point and attach
            // every two verices to the center as triangles.

            glm::vec3 center = {0.0f, 0.0f, 0.0f};
            for (int vert = 0; vert < verts; ++vert)
            {
                center += GetFaceVertex(mesh, face, vert);
            }
            center /= verts;

            for (int vert = 0; vert < verts - 1; ++vert)
            {

                glm::vec3 vs[] =
                {
                    GetFaceVertex(mesh, face, vert),
                    GetFaceVertex(mesh, face, vert + 1),
                    center,
                };
                glm::vec3 normal = TriangleNormal(vs);

                push(vs[0], normal);
                push(vs[1], normal);
                push(vs[2], normal);
            }

            // complete the polygon loop
            glm::vec3 vs[] =
            {
                GetFaceVertex(mesh, face, verts - 1),
                GetFaceVertex(mesh, face, 0),
                center,
            };
            glm::vec3 normal = TriangleNormal(vs);

            push(vs[0], normal);
            push(vs[1], normal);
            push(vs[2], normal);
        }
    }

    return Mesh{std::move(rv)};
}

osc::SystemDecorationNew::operator SystemDecoration() const
{
    glm::mat4 m = toMat4(transform);
    return SystemDecoration{
        mesh,
        m,
        toNormalMatrix(transform),
        color,
        AABBApplyXform(mesh->getAABB(), m)
    };
}


// osc::DecorativeGeometryHandler

osc::DecorativeGeometryHandler::DecorativeGeometryHandler(MeshCache& meshCache,
                                                          SimTK::SimbodyMatterSubsystem const& matter,
                                                          SimTK::State const& state,
                                                          float fixupScaleFactor,
                                                          std::function<void(SystemDecorationNew const&)>& callback) :
    m_Impl{std::make_unique<Impl>(meshCache, matter, state, std::move(fixupScaleFactor), callback)}
{
}

osc::DecorativeGeometryHandler::DecorativeGeometryHandler(DecorativeGeometryHandler&&) noexcept = default;

osc::DecorativeGeometryHandler::~DecorativeGeometryHandler() noexcept = default;

osc::DecorativeGeometryHandler& osc::DecorativeGeometryHandler::operator=(DecorativeGeometryHandler&&) noexcept = default;

void osc::DecorativeGeometryHandler::operator()(SimTK::DecorativeGeometry const& dg)
{
    dg.implementGeometry(*m_Impl);
}

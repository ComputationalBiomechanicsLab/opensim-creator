#include "SimTKHelpers.hpp"

#include "src/Graphics/Renderer.hpp"
#include "src/Graphics/MeshCache.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/Maths/Segment.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Platform/Log.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <simbody/internal/common.h>
#include <simbody/internal/MobilizedBody.h>
#include <simbody/internal/SimbodyMatterSubsystem.h>
#include <SimTKcommon/internal/DecorativeGeometry.h>
#include <SimTKcommon/internal/MassProperties.h>
#include <SimTKcommon/internal/Rotation.h>
#include <SimTKcommon/internal/State.h>
#include <SimTKcommon/internal/Transform.h>
#include <SimTKcommon/internal/PolygonalMesh.h>
#include <SimTKcommon/SmallMatrix.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <utility>
#include <vector>

static inline constexpr float g_LineThickness = 0.005f;
static inline constexpr float g_FrameAxisLengthRescale = 0.25f;
static inline constexpr float g_FrameAxisThickness = 0.0025f;

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

// extracts RGBA from geometry
static glm::vec4 GetColor(SimTK::DecorativeGeometry const& geom)
{
    SimTK::Vec3 const& rgb = geom.getColor();
    float ar = static_cast<float>(geom.getOpacity());
    ar = ar < 0.0f ? 1.0f : ar;
    return osc::ToVec4(rgb, ar);
}

// creates a geometry-to-ground transform for the given geometry
static osc::Transform ToOscTransform(
    SimTK::SimbodyMatterSubsystem const& matter,
    SimTK::State const& state,
    SimTK::DecorativeGeometry const& g)
{
    SimTK::MobilizedBody const& mobod = matter.getMobilizedBody(SimTK::MobilizedBodyIndex(g.getBodyId()));
    SimTK::Transform const& body2ground = mobod.getBodyTransform(state);
    SimTK::Transform const& decoration2body = g.getTransform();

    osc::Transform rv = osc::ToTransform(body2ground * decoration2body);
    rv.scale = GetScaleFactors(g);

    return rv;
}

// get the `vert`th vertex of the `face`th face
static glm::vec3 GetFaceVertex(SimTK::PolygonalMesh const& mesh, int face, int vert)
{
    int vertidx = mesh.getFaceVertex(face, vert);
    SimTK::Vec3 const& data = mesh.getVertexPosition(vertidx);
    return osc::ToVec3(data);
}

// an implementation of SimTK::DecorativeGeometryImplementation that emits generic
// triangle-mesh-based SystemDecorations that can be consumed by the rest of the UI
class osc::DecorativeGeometryHandler::Impl final : public SimTK::DecorativeGeometryImplementation {
public:
    Impl(MeshCache& meshCache,
         SimTK::SimbodyMatterSubsystem const& matter,
         SimTK::State const& st,
         float fixupScaleFactor,
         DecorationConsumer& decorationConsumer) :

        m_MeshCache{meshCache},
        m_Matter{matter},
        m_St{st},
        m_FixupScaleFactor{fixupScaleFactor},
        m_Consumer{decorationConsumer}
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

        glm::vec3 p1 = TransformPoint(t, ToVec4(d.getPoint1()));
        glm::vec3 p2 = TransformPoint(t, ToVec4(d.getPoint2()));

        float thickness = g_LineThickness * m_FixupScaleFactor;

        Transform cylinderXform = SimbodyCylinderToSegmentTransform({p1, p2}, thickness);
        cylinderXform.scale *= t.scale;

        m_Consumer(m_MeshCache.getCylinderMesh(), cylinderXform, GetColor(d));
    }

    void implementBrickGeometry(SimTK::DecorativeBrick const& d) override
    {
        Transform t = ToOscTransform(d);
        t.scale *= ToVec3(d.getHalfLengths());

        m_Consumer(m_MeshCache.getBrickMesh(), t, GetColor(d));
    }

    void implementCylinderGeometry(SimTK::DecorativeCylinder const& d) override
    {
        float radius = static_cast<float>(d.getRadius());

        Transform t = ToOscTransform(d);
        t.scale.x *= radius;
        t.scale.y *= static_cast<float>(d.getHalfHeight());
        t.scale.z *= radius;

        m_Consumer(m_MeshCache.getCylinderMesh(), t, GetColor(d));
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

        m_Consumer(m_MeshCache.getSphereMesh(), t, GetColor(d));
    }

    void implementEllipsoidGeometry(SimTK::DecorativeEllipsoid const& d) override
    {
        Transform t = ToOscTransform(d);
        t.scale *= ToVec3(d.getRadii());

        m_Consumer(m_MeshCache.getSphereMesh(), t, GetColor(d));
    }

    void implementFrameGeometry(SimTK::DecorativeFrame const& d) override
    {
        Transform t = ToOscTransform(d);

        // emit origin sphere
        {
            float r = 0.05f * g_FrameAxisLengthRescale * m_FixupScaleFactor;
            Transform sphereXform = t.withScale(r);
            glm::vec4 white = {1.0f, 1.0f, 1.0f, 1.0f};

            m_Consumer(m_MeshCache.getSphereMesh(), sphereXform, white);
        }

        // emit leg cylinders
        glm::vec3 axisLengths = t.scale * static_cast<float>(d.getAxisLength());
        float legLen = g_FrameAxisLengthRescale * m_FixupScaleFactor;
        float legThickness = g_FrameAxisThickness * m_FixupScaleFactor;
        for (int axis = 0; axis < 3; ++axis)
        {
            glm::vec3 dir = {0.0f, 0.0f, 0.0f};
            dir[axis] = legLen * axisLengths[axis];

            Segment line{t.position, t.position + TransformDirection(t, dir)};
            Transform legXform = SimbodyCylinderToSegmentTransform(line, legThickness);

            glm::vec4 color = {0.0f, 0.0f, 0.0f, 1.0f};
            color[axis] = 1.0f;

            m_Consumer(m_MeshCache.getCylinderMesh(), legXform, color);
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
        m_Consumer(m_MeshCache.getMeshFile(d.getMeshFile()), ToOscTransform(d), GetColor(d));
    }

    void implementArrowGeometry(SimTK::DecorativeArrow const& d) override
    {
        Transform t = ToOscTransform(d);

        glm::vec3 startBase = ToVec3(d.getStartPoint());
        glm::vec3 endBase = ToVec3(d.getEndPoint());

        glm::vec3 start = TransformPoint(t, startBase);
        glm::vec3 end = TransformPoint(t, endBase);

        glm::vec3 dir = glm::normalize(end - start);

        glm::vec3 neckStart = start;
        glm::vec3 neckEnd = end - (static_cast<float>(d.getTipLength()) * dir);
        glm::vec3 const& headStart = neckEnd;
        glm::vec3 const& headEnd = end;

        constexpr float neckThickness = 0.005f;
        constexpr float headThickness = 0.02f;

        glm::vec4 color = GetColor(d);

        // emit neck cylinder
        Transform neckXform = SimbodyCylinderToSegmentTransform({neckStart, neckEnd}, neckThickness);
        m_Consumer(m_MeshCache.getCylinderMesh(), neckXform, color);

        // emit head cone
        Transform headXform = SimbodyCylinderToSegmentTransform({headStart, headEnd}, headThickness);
        m_Consumer(m_MeshCache.getConeMesh(), headXform, color);
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

        glm::vec3 posBase = ToVec3(d.getOrigin());
        glm::vec3 posDir = ToVec3(d.getDirection());

        glm::vec3 pos = TransformPoint(t, posBase);
        glm::vec3 dir = TransformDirection(t, posDir);

        float radius = static_cast<float>(d.getBaseRadius());
        float height = static_cast<float>(d.getHeight());

        Transform coneXform = SimbodyCylinderToSegmentTransform({pos, pos + height*dir}, radius);
        coneXform.scale *= t.scale;

        m_Consumer(m_MeshCache.getConeMesh(), coneXform, GetColor(d));
    }

    MeshCache& m_MeshCache;
    SimTK::SimbodyMatterSubsystem const& m_Matter;
    SimTK::State const& m_St;
    float m_FixupScaleFactor;
    DecorationConsumer& m_Consumer;
};


// public API

SimTK::Vec3 osc::ToSimTKVec3(float v[3])
{
    return
    {
        static_cast<double>(v[0]),
        static_cast<double>(v[1]),
        static_cast<double>(v[2]),
    };
}

SimTK::Vec3 osc::ToSimTKVec3(glm::vec3 const& v)
{
    return
    {
        static_cast<double>(v.x),
        static_cast<double>(v.y),
        static_cast<double>(v.z),
    };
}

SimTK::Mat33 osc::ToSimTKMat3(glm::mat3 const& m)
{
    return SimTK::Mat33{
        static_cast<double>(m[0][0]), static_cast<double>(m[1][0]), static_cast<double>(m[2][0]),
        static_cast<double>(m[0][1]), static_cast<double>(m[1][1]), static_cast<double>(m[2][1]),
        static_cast<double>(m[0][2]), static_cast<double>(m[1][2]), static_cast<double>(m[2][2])
    };
}

SimTK::Inertia osc::ToSimTKInertia(float v[3])
{
    return
    {
        static_cast<double>(v[0]),
        static_cast<double>(v[1]),
        static_cast<double>(v[2]),
    };
}

SimTK::Inertia osc::ToSimTKInertia(glm::vec3 const& v)
{
    return
    {
        static_cast<double>(v[0]),
        static_cast<double>(v[1]),
        static_cast<double>(v[2]),
    };
}

SimTK::Transform osc::ToSimTKTransform(glm::mat4x3 const& m)
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

SimTK::Transform osc::ToSimTKTransform(Transform const& t)
{
    return SimTK::Transform{ToSimTKRotation(t.rotation), ToSimTKVec3(t.position)};
}

SimTK::Rotation osc::ToSimTKRotation(glm::quat const& q)
{
    return SimTK::Rotation{ToSimTKMat3(glm::toMat3(q))};
}

glm::vec3 osc::ToVec3(SimTK::Vec3 const& v)
{
    return glm::vec3
    {
        static_cast<float>(v[0]),
        static_cast<float>(v[1]),
        static_cast<float>(v[2])
    };
}

glm::vec4 osc::ToVec4(SimTK::Vec3 const& v, float w)
{
    return glm::vec4
    {
        static_cast<float>(v[0]),
        static_cast<float>(v[1]),
        static_cast<float>(v[2]),
        static_cast<float>(w),
    };
}

glm::mat4x3 osc::ToMat4x3(SimTK::Transform const& t)
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

glm::mat4x4 osc::ToMat4x4(SimTK::Transform const& t)
{
    return glm::mat4{ToMat4x3(t)};
}

glm::quat osc::ToQuat(SimTK::Rotation const& r)
{
    SimTK::Quaternion q = r.convertRotationToQuaternion();

    return glm::quat
    {
        static_cast<float>(q[0]),
        static_cast<float>(q[1]),
        static_cast<float>(q[2]),
        static_cast<float>(q[3]),
    };
}

osc::Transform osc::ToTransform(SimTK::Transform const& t)
{
    return Transform{ToVec3(t.p()), ToQuat(t.R())};
}


// mesh loading

osc::experimental::Mesh osc::LoadMeshViaSimTK(std::filesystem::path const& p)
{
    SimTK::DecorativeMeshFile dmf{p.string()};
    SimTK::PolygonalMesh const& mesh = dmf.getMesh();

    std::vector<glm::vec3> verts;
    std::vector<glm::vec3> normals;
    std::vector<uint32_t> indices;

    verts.reserve(static_cast<size_t>(mesh.getNumVertices()));
    normals.reserve(static_cast<size_t>(mesh.getNumVertices()));
    indices.reserve(static_cast<size_t>(mesh.getNumVertices()));

    uint32_t index = 0;
    auto push = [&verts, &normals, &indices, &index](glm::vec3 const& pos, glm::vec3 const& normal)
    {
        verts.push_back(pos);
        normals.push_back(normal);
        indices.push_back(index++);
    };

    for (int face = 0, nfaces = mesh.getNumFaces(); face < nfaces; ++face)
    {
        int nVerts = mesh.getNumVerticesForFace(face);

        if (nVerts < 3)
        {
            // line/point

            // ignore

        }
        else if (nVerts == 3)
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
        else if (nVerts == 4)
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
            for (int vert = 0; vert < nVerts; ++vert)
            {
                center += GetFaceVertex(mesh, face, vert);
            }
            center /= nVerts;

            for (int vert = 0; vert < nVerts - 1; ++vert)
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
                GetFaceVertex(mesh, face, nVerts - 1),
                GetFaceVertex(mesh, face, 0),
                center,
            };
            glm::vec3 normal = TriangleNormal(vs);

            push(vs[0], normal);
            push(vs[1], normal);
            push(vs[2], normal);
        }
    }

    experimental::Mesh rv;
    rv.setTopography(experimental::MeshTopography::Triangles);
    rv.setVerts(verts);
    rv.setNormals(normals);
    rv.setIndices(indices);
    return rv;
}



// osc::DecorativeGeometryHandler

osc::DecorativeGeometryHandler::DecorativeGeometryHandler(MeshCache& meshCache,
                                                          SimTK::SimbodyMatterSubsystem const& matter,
                                                          SimTK::State const& state,
                                                          float fixupScaleFactor,
                                                          DecorationConsumer& decorationConsumer) :
    m_Impl{new Impl{meshCache, matter, state, std::move(fixupScaleFactor), decorationConsumer}}
{
}

osc::DecorativeGeometryHandler::DecorativeGeometryHandler(DecorativeGeometryHandler&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::DecorativeGeometryHandler& osc::DecorativeGeometryHandler::operator=(DecorativeGeometryHandler&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::DecorativeGeometryHandler::~DecorativeGeometryHandler() noexcept
{
    delete m_Impl;
}

void osc::DecorativeGeometryHandler::operator()(SimTK::DecorativeGeometry const& dg)
{
    dg.implementGeometry(*m_Impl);
}

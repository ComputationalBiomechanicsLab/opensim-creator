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

// extract scale factors from geometry
static glm::vec3 scaleFactors(SimTK::DecorativeGeometry const& geom) {
    SimTK::Vec3 sf = geom.getScaleFactors();
    for (int i = 0; i < 3; ++i) {
        sf[i] = sf[i] <= 0 ? 1.0 : sf[i];
    }
    return glm::vec3{sf[0], sf[1], sf[2]};
}
// extract RGBA from geometry
static glm::vec4 extractRGBA(SimTK::DecorativeGeometry const& geom) {
    SimTK::Vec3 const& rgb = geom.getColor();
    SimTK::Real ar = geom.getOpacity();
    ar = ar < 0.0 ? 1.0 : ar;
    return glm::vec4(rgb[0], rgb[1], rgb[2], ar);
}

// get modelspace to worldspace xform for a given decorative element
static glm::mat4 geomXform(SimTK::SimbodyMatterSubsystem const& matter, SimTK::State const& state, SimTK::DecorativeGeometry const& g) {
    SimTK::MobilizedBody const& mobod = matter.getMobilizedBody(SimTK::MobilizedBodyIndex(g.getBodyId()));
    glm::mat4 ground2body = SimTKMat4x4FromTransform(mobod.getBodyTransform(state));
    glm::mat4 body2decoration = SimTKMat4x4FromTransform(g.getTransform());
    return ground2body * body2decoration;
}

static glm::vec3 getFaceVertex(SimTK::PolygonalMesh const& mesh, int face, int vert)
{
    int vertidx = mesh.getFaceVertex(face, vert);
    SimTK::Vec3 const& data = mesh.getVertexPosition(vertidx);
    return SimTKVec3FromVec3(data);
}

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
    auto push = [&rv, &index](glm::vec3 const& pos, glm::vec3 const& normal) {
        rv.verts.push_back(pos);
        rv.normals.push_back(normal);
        rv.indices.push_back(index++);
    };

    for (int face = 0, nfaces = mesh.getNumFaces(); face < nfaces; ++face) {
        int verts = mesh.getNumVerticesForFace(face);

        if (verts < 3) {
            // line/point

            // ignore

        } else if (verts == 3) {
            // triangle

            glm::vec3 vs[] = {
                getFaceVertex(mesh, face, 0),
                getFaceVertex(mesh, face, 1),
                getFaceVertex(mesh, face, 2),
            };
            glm::vec3 normal = TriangleNormal(vs);

            push(vs[0], normal);
            push(vs[1], normal);
            push(vs[2], normal);

        } else if (verts == 4) {
            // quad: render as two triangles

            glm::vec3 vs[] = {
                getFaceVertex(mesh, face, 0),
                getFaceVertex(mesh, face, 1),
                getFaceVertex(mesh, face, 2),
                getFaceVertex(mesh, face, 3),
            };

            glm::vec3 norms[] = {
                TriangleNormal(vs[0], vs[1], vs[2]),
                TriangleNormal(vs[2], vs[3], vs[0]),
            };

            push(vs[0], norms[0]);
            push(vs[1], norms[0]);
            push(vs[2], norms[0]);
            push(vs[2], norms[1]);
            push(vs[3], norms[1]);
            push(vs[0], norms[1]);

        } else {
            // polygon (>3 edges):
            //
            // create a vertex at the average center point and attach
            // every two verices to the center as triangles.

            glm::vec3 center = {0.0f, 0.0f, 0.0f};
            for (int vert = 0; vert < verts; ++vert) {
                center += getFaceVertex(mesh, face, vert);
            }
            center /= verts;

            for (int vert = 0; vert < verts - 1; ++vert) {

                glm::vec3 vs[] = {
                    getFaceVertex(mesh, face, vert),
                    getFaceVertex(mesh, face, vert + 1),
                    center,
                };
                glm::vec3 normal = TriangleNormal(vs);

                push(vs[0], normal);
                push(vs[1], normal);
                push(vs[2], normal);
            }

            // complete the polygon loop
            glm::vec3 vs[] = {
                getFaceVertex(mesh, face, verts - 1),
                getFaceVertex(mesh, face, 0),
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

osc::SceneGeneratorNew::SceneGeneratorNew(MeshCache& meshCache,
                                          SimTK::SimbodyMatterSubsystem const& matter,
                                          SimTK::State const& st,
                                          float fixupScaleFactor) :
    m_MeshCache{meshCache},
    m_Matter{matter},
    m_St{st},
    m_FixupScaleFactor{fixupScaleFactor} {
}

void osc::SceneGeneratorNew::implementPointGeometry(SimTK::DecorativePoint const&) {
    static bool shown_once = []() {
        log::warn("this model uses implementPointGeometry, which is not yet implemented in OSC");
        return true;
    }();
    (void)shown_once;
}

void osc::SceneGeneratorNew::implementLineGeometry(SimTK::DecorativeLine const& dl) {
    glm::mat4 m = geomXform(m_Matter, m_St, dl);

    glm::vec4 p1 = m * SimTKVec4FromVec3(dl.getPoint1());
    glm::vec4 p2 = m * SimTKVec4FromVec3(dl.getPoint2());

    Segment meshLine{{0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    Segment emittedLine{p1, p2};

    glm::mat4 cylinderXform = SegmentToSegmentXform(meshLine, emittedLine);
    glm::mat4 scaler =
        glm::scale(glm::mat4{1.0f}, glm::vec3{g_LineThickness * m_FixupScaleFactor, 1.0f, g_LineThickness * m_FixupScaleFactor} * scaleFactors(dl));

    SystemDecoration se;
    se.mesh = m_MeshCache.getCylinderMesh();
    se.modelMtx = glm::mat4x3{cylinderXform * scaler};
    se.normalMtx = NormalMatrix(se.modelMtx);
    se.color = extractRGBA(dl);
    se.worldspaceAABB = AABBApplyXform(se.mesh->getAABB(), se.modelMtx);

    onSceneElementEmission(se);
}

void osc::SceneGeneratorNew::implementBrickGeometry(SimTK::DecorativeBrick const& db) {
    glm::vec3 halfdims = SimTKVec3FromVec3(db.getHalfLengths());

    SystemDecoration se;
    se.mesh = m_MeshCache.getBrickMesh();
    se.modelMtx = glm::scale(geomXform(m_Matter, m_St, db), halfdims * scaleFactors(db));
    se.normalMtx = NormalMatrix(se.modelMtx);
    se.color = extractRGBA(db);
    se.worldspaceAABB = AABBApplyXform(se.mesh->getAABB(), se.modelMtx);

    onSceneElementEmission(se);
}

void osc::SceneGeneratorNew::implementCylinderGeometry(SimTK::DecorativeCylinder const& dc) {
    glm::vec3 s = scaleFactors(dc);
    s.x *= static_cast<float>(dc.getRadius());
    s.y *= static_cast<float>(dc.getHalfHeight());
    s.z *= static_cast<float>(dc.getRadius());

    SystemDecoration se;
    se.mesh = m_MeshCache.getCylinderMesh();
    se.modelMtx = glm::scale(geomXform(m_Matter, m_St, dc), s);
    se.normalMtx = NormalMatrix(se.modelMtx);
    se.color = extractRGBA(dc);
    se.worldspaceAABB = AABBApplyXform(se.mesh->getAABB(), se.modelMtx);

    onSceneElementEmission(se);
}

void osc::SceneGeneratorNew::implementCircleGeometry(SimTK::DecorativeCircle const&) {
    static bool shownWarning = []() {
        log::warn("this model uses implementCircleGeometry, which is not yet implemented in OSC");
        return true;
    }();
    (void)shownWarning;
}

void osc::SceneGeneratorNew::implementSphereGeometry(SimTK::DecorativeSphere const& ds) {
    glm::mat4 baseXform = geomXform(m_Matter, m_St, ds);
    glm::vec3 pos{baseXform[3][0], baseXform[3][1], baseXform[3][2]};  // scale factors are ignored, sorry not sorry, etc.

    // this code is fairly custom to make it faster
    //
    // - OpenSim scenes typically contain *a lot* of spheres
    // - it's much cheaper to compute things like normal matrices and AABBs when
    //   you know it's a sphere
    float scaledR = m_FixupScaleFactor * static_cast<float>(ds.getRadius());
    glm::vec3 sfs = scaleFactors(ds);
    glm::mat4 xform;
    xform[0] = {scaledR * sfs[0], 0.0f, 0.0f, 0.0f};
    xform[1] = {0.0f, scaledR * sfs[1], 0.0f, 0.0f};
    xform[2] = {0.0f, 0.0f, scaledR * sfs[2], 0.0f};
    xform[3] = glm::vec4{pos, 1.0f};
    glm::mat4 normalXform = glm::transpose(xform);
    AABB aabb = SphereToAABB(Sphere{pos, scaledR});

    SystemDecoration se;
    se.mesh = m_MeshCache.getSphereMesh();
    se.modelMtx = xform;
    se.normalMtx = normalXform;
    se.color = extractRGBA(ds);
    se.worldspaceAABB = aabb;

    onSceneElementEmission(se);
}

void osc::SceneGeneratorNew::implementEllipsoidGeometry(SimTK::DecorativeEllipsoid const& de) {
    glm::mat4 xform = geomXform(m_Matter, m_St, de);
    glm::vec3 sfs = scaleFactors(de);
    glm::vec3 radii = SimTKVec3FromVec3(de.getRadii());

    SystemDecoration se;
    se.mesh = m_MeshCache.getSphereMesh();
    se.modelMtx = glm::scale(xform, sfs * radii);
    se.normalMtx = NormalMatrix(se.modelMtx);
    se.color = extractRGBA(de);
    se.worldspaceAABB = AABBApplyXform(se.mesh->getAABB(), xform);

    onSceneElementEmission(se);
}

void osc::SceneGeneratorNew::implementFrameGeometry(SimTK::DecorativeFrame const& df) {
    glm::mat4 rawXform = geomXform(m_Matter, m_St, df);

    glm::vec3 pos{rawXform[3]};
    glm::mat3 rotation_mtx{rawXform};

    glm::vec3 axisLengths = scaleFactors(df) * static_cast<float>(df.getAxisLength());

    // emit origin sphere
    {
        Sphere meshSphere{{0.0f, 0.0f, 0.0f}, 1.0f};
        Sphere outputSphere{pos, 0.05f * g_FrameAxisLengthRescale * m_FixupScaleFactor};

        SystemDecoration se;
        se.mesh = m_MeshCache.getSphereMesh();
        se.modelMtx = SphereToSphereXform(meshSphere, outputSphere);
        se.normalMtx = NormalMatrix(se.modelMtx);
        se.color = {1.0f, 1.0f, 1.0f, 1.0f};
        se.worldspaceAABB = AABBApplyXform(se.mesh->getAABB(), se.modelMtx);

        onSceneElementEmission(se);
    }

    // emit axis cylinders
    Segment cylinderline{{0.0f, -1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}};
    for (int i = 0; i < 3; ++i) {
        glm::vec3 dir = {0.0f, 0.0f, 0.0f};
        dir[i] = g_FrameAxisLengthRescale * m_FixupScaleFactor * axisLengths[i];
        Segment axisline{pos, pos + rotation_mtx*dir};

        glm::vec3 prescale = {g_FrameAxisThickness * m_FixupScaleFactor, 1.0f, g_FrameAxisThickness * m_FixupScaleFactor};
        glm::mat4 prescaleMtx = glm::scale(glm::mat4{1.0f}, prescale);
        glm::vec4 color{0.0f, 0.0f, 0.0f, 1.0f};
        color[i] = 1.0f;

        SystemDecoration se;
        se.mesh = m_MeshCache.getCylinderMesh();
        se.modelMtx = SegmentToSegmentXform(cylinderline, axisline) * prescaleMtx;
        se.normalMtx = NormalMatrix(se.modelMtx);
        se.color = color;
        se.worldspaceAABB = AABBApplyXform(se.mesh->getAABB(), se.modelMtx);

        onSceneElementEmission(se);
    }
}

void osc::SceneGeneratorNew::implementTextGeometry(SimTK::DecorativeText const&) {
    static bool shownWarning = []() {
        log::warn("this model uses implementTextGeometry, which is not yet implemented in OSC");
        return true;
    }();
    (void)shownWarning;
}

void osc::SceneGeneratorNew::implementMeshGeometry(SimTK::DecorativeMesh const&) {
    static bool shownWarning = []() {
        log::warn("this model uses implementMeshGeometry, which is not yet implemented in OSC");
        return true;
    }();
    (void)shownWarning;
}

void osc::SceneGeneratorNew::implementMeshFileGeometry(SimTK::DecorativeMeshFile const& dmf) {
    SystemDecoration se;
    se.mesh = m_MeshCache.getMeshFile(dmf.getMeshFile());
    se.modelMtx = glm::scale(geomXform(m_Matter, m_St, dmf), scaleFactors(dmf));
    se.normalMtx = NormalMatrix(se.modelMtx);
    se.color = extractRGBA(dmf);
    se.worldspaceAABB = AABBApplyXform(se.mesh->getAABB(), se.modelMtx);

    onSceneElementEmission(se);
}

void osc::SceneGeneratorNew::implementArrowGeometry(SimTK::DecorativeArrow const& da) {
    glm::mat4 xform = glm::scale(geomXform(m_Matter, m_St, da), scaleFactors(da));

    glm::vec3 baseStartpoint = SimTKVec3FromVec3(da.getStartPoint());
    glm::vec3 baseEndpoint = SimTKVec3FromVec3(da.getEndPoint());

    glm::vec3 p1 = glm::vec3{xform * glm::vec4{baseStartpoint, 1.0f}};
    glm::vec3 p2 = glm::vec3{xform * glm::vec4{baseEndpoint, 1.0f}};
    glm::vec3 p1_to_p2 = p2 - p1;

    float len = glm::length(p1_to_p2);
    glm::vec3 dir = p1_to_p2/len;

    Segment meshline{{0.0f, -1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}};
    glm::vec3 cylinder_start = p1;
    glm::vec3 cone_start = p2 - (g_ConeHeadLength * len * dir);
    glm::vec3 cone_end = p2;

    // emit arrow head (a cone)
    {
        glm::mat4 cone_radius_rescaler = glm::scale(glm::mat4{1.0f}, {0.02f, 1.0f, 0.02f});

        SystemDecoration se;
        se.mesh = m_MeshCache.getConeMesh();
        se.modelMtx = SegmentToSegmentXform(meshline, Segment{cone_start, cone_end}) * cone_radius_rescaler;
        se.normalMtx = NormalMatrix(se.modelMtx);
        se.color = extractRGBA(da);
        se.worldspaceAABB = AABBApplyXform(se.mesh->getAABB(), se.modelMtx);

        onSceneElementEmission(se);
    }

    // emit arrow tail (a cylinder)
    {
        glm::mat4 cylinder_radius_rescaler = glm::scale(glm::mat4{1.0f}, {0.005f, 1.0f, 0.005f});

        SystemDecoration se;
        se.mesh = m_MeshCache.getCylinderMesh();
        se.modelMtx = SegmentToSegmentXform(meshline, Segment{cylinder_start, cone_start}) * cylinder_radius_rescaler;
        se.normalMtx = NormalMatrix(se.modelMtx);
        se.color = extractRGBA(da);
        se.worldspaceAABB = AABBApplyXform(se.mesh->getAABB(), se.modelMtx);

        onSceneElementEmission(se);
    }
}

void osc::SceneGeneratorNew::implementTorusGeometry(SimTK::DecorativeTorus const&) {
    static bool shownWarning = []() {
        log::warn("this model uses implementTorusGeometry, which is not yet implemented in OSC");
        return true;
    }();
    (void)shownWarning;
}

void osc::SceneGeneratorNew::implementConeGeometry(SimTK::DecorativeCone const& dc)
{
    glm::mat4 xform = glm::scale(geomXform(m_Matter, m_St, dc), scaleFactors(dc));

    glm::vec3 basePos = SimTKVec3FromVec3(dc.getOrigin());
    glm::vec3 baseDir = SimTKVec3FromVec3(dc.getDirection());

    glm::vec3 worldPos = glm::vec3{xform * glm::vec4{basePos, 1.0f}};
    glm::vec3 worldDir = glm::normalize(glm::vec3{xform * glm::vec4{baseDir, 0.0f}});

    float baseRadius = static_cast<float>(dc.getBaseRadius());
    float height = static_cast<float>(dc.getHeight());

    Segment meshline{{0.0f, -1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}};
    Segment coneline{worldPos, worldPos + worldDir*height};
    glm::mat4 lineXform = SegmentToSegmentXform(meshline, coneline);
    glm::mat4 radiusRescale = glm::scale(glm::mat4{1.0f}, {baseRadius, 1.0f, baseRadius});

    SystemDecoration se;
    se.mesh = m_MeshCache.getConeMesh();
    se.modelMtx = lineXform * radiusRescale;
    se.normalMtx = NormalMatrix(se.modelMtx);
    se.color = extractRGBA(dc);
    se.worldspaceAABB = AABBApplyXform(se.mesh->getAABB(), se.modelMtx);

    onSceneElementEmission(se);
}

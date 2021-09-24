#include "SceneGeneratorNew.hpp"

#include "src/SimTKBindings/SimTKConverters.hpp"
#include "src/Log.hpp"
#include "src/MeshCache.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <simbody/internal/SimbodyMatterSubsystem.h>

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
    glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, {g_LineThickness * m_FixupScaleFactor, 1.0f, g_LineThickness * m_FixupScaleFactor});

    SceneElement se;
    se.mesh = m_MeshCache.getCylinderMesh();
    se.modelMtx = glm::mat4x3{cylinderXform * scaler};
    se.normalMtx = NormalMatrix(se.modelMtx);
    se.color = extractRGBA(dl);
    se.worldspaceAABB = AABBApplyXform(se.mesh->getAABB(), se.modelMtx);

    onSceneElementEmission(se);
}

void osc::SceneGeneratorNew::implementBrickGeometry(SimTK::DecorativeBrick const& db) {
    glm::vec3 halfdims = SimTKVec3FromVec3(db.getHalfLengths());

    SceneElement se;
    se.mesh = m_MeshCache.getBrickMesh();
    se.modelMtx = glm::scale(geomXform(m_Matter, m_St, db), halfdims);
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

    SceneElement se;
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
    glm::mat4 xform;
    xform[0] = {scaledR, 0.0f, 0.0f, 0.0f};
    xform[1] = {0.0f, scaledR, 0.0f, 0.0f};
    xform[2] = {0.0f, 0.0f, scaledR, 0.0f};
    xform[3] = glm::vec4{pos, 1.0f};
    glm::mat4 normalXform = glm::transpose(xform);
    AABB aabb = SphereToAABB(Sphere{pos, scaledR});

    SceneElement se;
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

    SceneElement se;
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

        SceneElement se;
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

        SceneElement se;
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
    SceneElement se;
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

        SceneElement se;
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

        SceneElement se;
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

void osc::SceneGeneratorNew::implementConeGeometry(SimTK::DecorativeCone const& dc) {
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

    SceneElement se;
    se.mesh = m_MeshCache.getConeMesh();
    se.modelMtx = lineXform * radiusRescale;
    se.normalMtx = NormalMatrix(se.modelMtx);
    se.color = extractRGBA(dc);
    se.worldspaceAABB = AABBApplyXform(se.mesh->getAABB(), se.modelMtx);

    onSceneElementEmission(se);
}

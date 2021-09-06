#include "SceneGenerator.hpp"

#include "src/Assertions.hpp"
#include "src/Log.hpp"
#include "src/3d/BVH.hpp"
#include "src/3d/InstancedRenderer.hpp"
#include "src/3d/Model.hpp"
#include "src/simtk_bindings/SimTKGeometryGenerator.hpp"
#include "src/simtk_bindings/SimTKLoadMesh.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ModelDisplayHints.h>
#include <SimTKcommon.h>

#include <memory>

using namespace osc;

static constexpr char const* g_SphereID = "SPHERE_MESH";
static constexpr char const* g_CylinderID = "CYLINDER_MESH";
static constexpr char const* g_BrickID = "BRICK_MESH";
static constexpr char const* g_ConeID = "CONE_MESH";
static constexpr float g_LineThickness = 0.005f;
static constexpr float g_FrameAxisLengthRescale = 0.25f;
static constexpr float g_FrameAxisThickness = 0.0025f;

namespace {
    // common data that's handed to each emission function
    struct EmitterOut final {
        // meshdata
        std::unordered_map<std::string, std::unique_ptr<CachedMeshdata>>& meshCache;
        CachedMeshdata& sphere;
        CachedMeshdata& cylinder;
        CachedMeshdata& brick;
        CachedMeshdata& cone;

        // current component
        OpenSim::Component const* c;

        // output decoration list
        SceneDecorations& decs;

        // fixup scale factor for muscles, spheres, etc.
        float fixupScaleFactor;
    };
}

// preprocess (AABB, BVH, etc.) CPU-side data and upload it to the instanced renderer
static std::unique_ptr<CachedMeshdata> createCachedMeshdata(Mesh srcMesh) {

    // heap-allocate the meshdata into a shared pointer so that emitted drawlists can
    // independently contain a (reference-counted) pointer to the data
    std::shared_ptr<CPUMesh> cpumesh = std::make_shared<CPUMesh>();
    cpumesh->data = std::move(srcMesh);

    // precompute modelspace AABB
    cpumesh->aabb = AABBFromVerts(cpumesh->data.verts.data(), cpumesh->data.verts.size());

    // precompute modelspace BVH
    BVH_BuildFromTriangles(cpumesh->triangleBVH, cpumesh->data.verts.data(), cpumesh->data.verts.size());

    return std::unique_ptr<CachedMeshdata>{new CachedMeshdata{cpumesh, uploadMeshdataForInstancing(cpumesh->data)}};
}

static void handleLineEmission(SimbodyGeometry::Line const& l, EmitterOut& out) {
    SceneDecorations& dout = out.decs;

    Segment meshLine{{0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    Segment emittedLine{l.p1, l.p2};

    glm::mat4x3 const& xform = dout.modelMtxs.emplace_back(
        SegmentToSegmentXform(meshLine, emittedLine) * glm::scale(glm::mat4{1.0f}, {g_LineThickness * out.fixupScaleFactor, 1.0f, g_LineThickness * out.fixupScaleFactor}));
    dout.normalMtxs.emplace_back(NormalMatrix(xform));
    dout.cols.emplace_back(Rgba32FromVec4(l.rgba));
    dout.gpuMeshes.emplace_back(out.cylinder.instanceMeshdata);
    dout.cpuMeshes.emplace_back(out.cylinder.cpuMeshdata);
    dout.aabbs.emplace_back(AABBApplyXform(out.cylinder.cpuMeshdata->aabb, xform));
    dout.components.emplace_back(out.c);
}

static void handleCylinderEmission(SimbodyGeometry::Cylinder const& cy, EmitterOut& out) {
    SceneDecorations& dout = out.decs;

    glm::mat4x3 const& xform = dout.modelMtxs.emplace_back(cy.modelMtx);
    dout.normalMtxs.push_back(NormalMatrix(xform));
    dout.cols.push_back(Rgba32FromVec4(cy.rgba));
    dout.gpuMeshes.push_back(out.cylinder.instanceMeshdata);
    dout.cpuMeshes.push_back(out.cylinder.cpuMeshdata);
    dout.aabbs.push_back(AABBApplyXform(out.cylinder.cpuMeshdata->aabb, xform));
    dout.components.push_back(out.c);
}

static void handleConeEmission(SimbodyGeometry::Cone const& cone, EmitterOut& out) {
    SceneDecorations& dout = out.decs;

    Segment meshline{{0.0f, -1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}};
    Segment coneline{cone.pos, cone.pos + cone.direction*cone.height};
    glm::mat4 lineXform = SegmentToSegmentXform(meshline, coneline);
    glm::mat4 radiusRescale = glm::scale(glm::mat4{1.0f}, {cone.baseRadius, 1.0f, cone.baseRadius});

    glm::mat4x3 const& xform = dout.modelMtxs.emplace_back(lineXform * radiusRescale);
    dout.normalMtxs.push_back(NormalMatrix(xform));
    dout.cols.push_back(Rgba32FromVec4(cone.rgba));
    dout.gpuMeshes.push_back(out.cone.instanceMeshdata);
    dout.cpuMeshes.push_back(out.cone.cpuMeshdata);
    dout.aabbs.push_back(AABBApplyXform(out.cone.cpuMeshdata->aabb, xform));
    dout.components.push_back(out.c);
}

static void handleSphereEmission(SimbodyGeometry::Sphere const& s, EmitterOut& out) {
    SceneDecorations& sd = out.decs;

    // this code is fairly custom to make it faster
    //
    // - OpenSim scenes typically contain *a lot* of spheres
    // - it's much cheaper to compute things like normal matrices and AABBs when
    //   you know it's a sphere
    float scaledR = out.fixupScaleFactor * s.radius;
    glm::mat4 xform;
    xform[0] = {scaledR, 0.0f, 0.0f, 0.0f};
    xform[1] = {0.0f, scaledR, 0.0f, 0.0f};
    xform[2] = {0.0f, 0.0f, scaledR, 0.0f};
    xform[3] = glm::vec4{s.pos, 1.0f};
    glm::mat4 normalXform = glm::transpose(xform);
    AABB aabb = SphereToAABB(Sphere{s.pos, scaledR});

    sd.modelMtxs.emplace_back(xform);
    sd.normalMtxs.emplace_back(normalXform);
    sd.cols.emplace_back(Rgba32FromVec4(s.rgba));
    sd.gpuMeshes.emplace_back(out.sphere.instanceMeshdata);
    sd.cpuMeshes.emplace_back(out.sphere.cpuMeshdata);
    sd.aabbs.emplace_back(aabb);
    sd.components.push_back(out.c);
}

static void handleBrickEmission(SimbodyGeometry::Brick const& b, EmitterOut& out) {
    SceneDecorations& dout = out.decs;

    glm::mat4x3 const& xform = dout.modelMtxs.emplace_back(b.modelMtx);
    dout.normalMtxs.push_back(NormalMatrix(xform));
    dout.cols.push_back(Rgba32FromVec4(b.rgba));
    dout.gpuMeshes.push_back(out.brick.instanceMeshdata);
    dout.cpuMeshes.push_back(out.brick.cpuMeshdata);
    dout.aabbs.push_back(AABBApplyXform(out.brick.cpuMeshdata->aabb, xform));
    dout.components.push_back(out.c);
}

static void handleMeshfileEmission(SimbodyGeometry::MeshFile const& mf, EmitterOut& out) {

    auto [it, inserted] = out.meshCache.try_emplace(*mf.path, nullptr);

    if (inserted) {
        // mesh wasn't in the cache, go load it
        try {
            it->second = createCachedMeshdata(SimTKLoadMesh(*mf.path));
        } catch (...) {
            // problems loading it: ensure cache isn't corrupted with the nullptr
            out.meshCache.erase(it);
            throw;
        }
    }

    // not null, because of the above insertion check
    CachedMeshdata& cm = *it->second;

    SceneDecorations& sd = out.decs;

    glm::mat4x3 const& xform = sd.modelMtxs.emplace_back(mf.modelMtx);
    sd.normalMtxs.emplace_back(NormalMatrix(xform));
    sd.cols.emplace_back(Rgba32FromVec4(mf.rgba));
    sd.gpuMeshes.emplace_back(cm.instanceMeshdata);
    sd.cpuMeshes.emplace_back(cm.cpuMeshdata);
    sd.aabbs.emplace_back(AABBApplyXform(cm.cpuMeshdata->aabb, xform));
    sd.components.emplace_back(out.c);
}

static void handleFrameEmission(SimbodyGeometry::Frame const& frame, EmitterOut& out) {
    SceneDecorations& dout = out.decs;

    // emit origin sphere
    {
        Sphere meshSphere{{0.0f, 0.0f, 0.0f}, 1.0f};
        Sphere outputSphere{frame.pos, 0.05f * g_FrameAxisLengthRescale * out.fixupScaleFactor};

        glm::mat4x3 const& xform = dout.modelMtxs.emplace_back(SphereToSphereXform(meshSphere, outputSphere));
        dout.normalMtxs.push_back(NormalMatrix(xform));
        dout.cols.push_back(Rgba32FromU32(0xffffffff));
        dout.gpuMeshes.push_back(out.sphere.instanceMeshdata);
        dout.cpuMeshes.push_back(out.sphere.cpuMeshdata);
        dout.aabbs.push_back(AABBApplyXform(out.sphere.cpuMeshdata->aabb, xform));
        dout.components.push_back(out.c);
    }

    // emit axis cylinders
    Segment cylinderline{{0.0f, -1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}};
    for (int i = 0; i < 3; ++i) {
        glm::vec3 dir = {0.0f, 0.0f, 0.0f};
        dir[i] = g_FrameAxisLengthRescale * out.fixupScaleFactor * frame.axisLengths[i];
        Segment axisline{frame.pos, frame.pos + dir};

        glm::vec3 prescale = {g_FrameAxisThickness * out.fixupScaleFactor, 1.0f, g_FrameAxisThickness * out.fixupScaleFactor};
        glm::mat4 prescaleMtx = glm::scale(glm::mat4{1.0f}, prescale);
        glm::vec4 color{0.0f, 0.0f, 0.0f, 1.0f};
        color[i] = 1.0f;

        glm::mat4x3 const& xform = dout.modelMtxs.emplace_back(SegmentToSegmentXform(cylinderline, axisline) * prescaleMtx);
        dout.normalMtxs.push_back(NormalMatrix(xform));
        dout.cols.push_back(Rgba32FromVec4(color));
        dout.gpuMeshes.push_back(out.cylinder.instanceMeshdata);
        dout.cpuMeshes.push_back(out.cylinder.cpuMeshdata);
        dout.aabbs.push_back(AABBApplyXform(out.cylinder.cpuMeshdata->aabb, xform));
        dout.components.push_back(out.c);
    }
}

static void handleEllipsoidEmission(SimbodyGeometry::Ellipsoid const& elip, EmitterOut& out) {
    SceneDecorations& sd = out.decs;

    glm::mat4x3 const& xform = sd.modelMtxs.emplace_back(elip.modelMtx);
    sd.normalMtxs.push_back(NormalMatrix(xform));
    sd.cols.push_back(Rgba32FromVec4(elip.rgba));
    sd.gpuMeshes.push_back(out.sphere.instanceMeshdata);
    sd.cpuMeshes.push_back(out.sphere.cpuMeshdata);
    sd.aabbs.push_back(AABBApplyXform(out.sphere.cpuMeshdata->aabb, xform));
    sd.components.push_back(out.c);
}

static void handleArrowEmission(SimbodyGeometry::Arrow const& a, EmitterOut& out) {
    SceneDecorations& dout = out.decs;

    glm::vec3 p1_to_p2 = a.p2 - a.p1;
    float len = glm::length(p1_to_p2);
    glm::vec3 dir = p1_to_p2/len;
    constexpr float conelen = 0.2f;

    Segment meshline{{0.0f, -1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}};
    glm::vec3 cylinder_start = a.p1;
    glm::vec3 cone_start = a.p2 - (conelen * len * dir);
    glm::vec3 cone_end = a.p2;

    // emit arrow's head (a cone)
    {
        glm::mat4 cone_radius_rescaler = glm::scale(glm::mat4{1.0f}, {0.02f, 1.0f, 0.02f});
        glm::mat4x3 const& xform =
            dout.modelMtxs.emplace_back(SegmentToSegmentXform(meshline, Segment{cone_start, cone_end}) * cone_radius_rescaler);
        dout.normalMtxs.push_back(NormalMatrix(xform));
        dout.cols.push_back(Rgba32FromVec4(a.rgba));
        dout.gpuMeshes.push_back(out.cone.instanceMeshdata);
        dout.cpuMeshes.push_back(out.cone.cpuMeshdata);
        dout.aabbs.push_back(AABBApplyXform(out.cone.cpuMeshdata->aabb, xform));
        dout.components.push_back(out.c);
    }

    // emit arrow's tail (a cylinder)
    {
        glm::mat4 cylinder_radius_rescaler = glm::scale(glm::mat4{1.0f}, {0.005f, 1.0f, 0.005f});
        glm::mat4x3 const& xform =
            dout.modelMtxs.emplace_back(SegmentToSegmentXform(meshline, Segment{cylinder_start, cone_start}) * cylinder_radius_rescaler);
        dout.normalMtxs.push_back(xform);
        dout.cols.push_back(Rgba32FromVec4(a.rgba));
        dout.gpuMeshes.push_back(out.cylinder.instanceMeshdata);
        dout.cpuMeshes.push_back(out.cylinder.cpuMeshdata);
        dout.aabbs.push_back(AABBApplyXform(out.cylinder.cpuMeshdata->aabb, xform));
        dout.components.push_back(out.c);
    }
}

// this is effectively called whenever OpenSim emits a decoration element
static void handleGeometryEmission(SimbodyGeometry const& g, EmitterOut& out) {
    switch (g.geometryType) {
    case SimbodyGeometry::Type::Sphere:
        handleSphereEmission(g.sphere, out);
        break;
    case SimbodyGeometry::Type::Line:
        handleLineEmission(g.line, out);
        break;
    case SimbodyGeometry::Type::Cylinder:
        handleCylinderEmission(g.cylinder, out);
        break;
    case SimbodyGeometry::Type::Brick:
        handleBrickEmission(g.brick, out);
        break;
    case SimbodyGeometry::Type::MeshFile:
        handleMeshfileEmission(g.meshfile, out);
        break;
    case SimbodyGeometry::Type::Frame:
        handleFrameEmission(g.frame, out);
        break;
    case SimbodyGeometry::Type::Ellipsoid:
        handleEllipsoidEmission(g.ellipsoid, out);
        break;
    case SimbodyGeometry::Type::Cone:
        handleConeEmission(g.cone, out);
        break;
    case SimbodyGeometry::Type::Arrow:
        handleArrowEmission(g.arrow, out);
        break;
    }
}

void osc::SceneDecorations::clear() {
    modelMtxs.clear();
    normalMtxs.clear();
    cols.clear();
    gpuMeshes.clear();
    cpuMeshes.clear();
    aabbs.clear();
    components.clear();
    sceneBVH.clear();
}

osc::SceneGenerator::SceneGenerator() {
    m_CachedMeshes[g_SphereID] = createCachedMeshdata(GenUntexturedUVSphere(12, 12));
    m_CachedMeshes[g_CylinderID] = createCachedMeshdata(GenUntexturedSimbodyCylinder(16));
    m_CachedMeshes[g_BrickID] = createCachedMeshdata(GenCube());
    m_CachedMeshes[g_ConeID] = createCachedMeshdata(GenUntexturedSimbodyCone(12));
}

void osc::SceneGenerator::generate(
        OpenSim::Component const& c,
        SimTK::State const& state,
        OpenSim::ModelDisplayHints const& hints,
        SceneGeneratorFlags flags,
        float fixupScaleFactor,
        SceneDecorations& out) {

    m_GeomListCache.clear();
    out.clear();

    // this is passed around a lot during emission - it's what each geometry emitter
    // uses to find/write things
    EmitterOut eo{
        m_CachedMeshes,
        *m_CachedMeshes[g_SphereID],
        *m_CachedMeshes[g_CylinderID],
        *m_CachedMeshes[g_BrickID],
        *m_CachedMeshes[g_ConeID],
        nullptr,
        out,
        fixupScaleFactor,
    };

    // called whenever OpenSim emits geometry
    auto onGeometryEmission = [&eo](SimbodyGeometry const& g) {
        handleGeometryEmission(g, eo);
    };

    // get component's matter subsystem
    SimTK::SimbodyMatterSubsystem const& matter = c.getSystem().getMatterSubsystem();

    // create a visitor that visits each component
    auto visitor = GeometryGeneratorLambda{matter, state, onGeometryEmission};

    // iterate through each component and walk through the geometry
    for (OpenSim::Component const& comp : c.getComponentList()) {
        eo.c = &comp;

        // emit static geometry (if requested)
        if (flags & SceneGeneratorFlags_GenerateStaticDecorations) {
            comp.generateDecorations(true, hints, state, m_GeomListCache);
            for (SimTK::DecorativeGeometry const& dg : m_GeomListCache) {
                dg.implementGeometry(visitor);
            }
            m_GeomListCache.clear();
        }

        // emit dynamic geometry (if requested)
        if (flags & SceneGeneratorFlags_GenerateDynamicDecorations) {
            comp.generateDecorations(false, hints, state, m_GeomListCache);
            for (SimTK::DecorativeGeometry const& dg : m_GeomListCache) {
                dg.implementGeometry(visitor);
            }
            m_GeomListCache.clear();
        }
    }

    OSC_ASSERT_ALWAYS(out.modelMtxs.size() == out.normalMtxs.size());
    OSC_ASSERT_ALWAYS(out.normalMtxs.size() == out.cols.size());
    OSC_ASSERT_ALWAYS(out.cols.size() == out.gpuMeshes.size());
    OSC_ASSERT_ALWAYS(out.gpuMeshes.size() == out.cpuMeshes.size());
    OSC_ASSERT_ALWAYS(out.cpuMeshes.size() == out.aabbs.size());

    // the geometry pass above populates everything but the scene BVH via the lambda
    BVH_BuildFromAABBs(out.sceneBVH, out.aabbs.data(), out.aabbs.size());
}

#include "scene_generator.hpp"

#include "src/assertions.hpp"
#include "src/log.hpp"
#include "src/3d/bvh.hpp"
#include "src/3d/instanced_renderer.hpp"
#include "src/3d/model.hpp"
#include "src/simtk_bindings/stk_geometry_generator.hpp"
#include "src/simtk_bindings/stk_meshloader.hpp"

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
    struct Emitter_out final {
        // meshdata
        std::unordered_map<std::string, std::unique_ptr<Cached_meshdata>>& mesh_cache;
        Cached_meshdata& sphere;
        Cached_meshdata& cylinder;
        Cached_meshdata& brick;
        Cached_meshdata& cone;

        // current component
        OpenSim::Component const* c;

        // output decoration list
        Scene_decorations& decs;

        // fixup scale factor for muscles, spheres, etc.
        float fixup_scale_factor;
    };
}

// preprocess (AABB, BVH, etc.) CPU-side data and upload it to the instanced renderer
static std::unique_ptr<Cached_meshdata> create_cached_meshdata(NewMesh src_mesh) {

    // heap-allocate the meshdata into a shared pointer so that emitted drawlists can
    // independently contain a (reference-counted) pointer to the data
    std::shared_ptr<CPU_mesh> cpumesh = std::make_shared<CPU_mesh>();
    cpumesh->data = std::move(src_mesh);

    // precompute modelspace AABB
    cpumesh->aabb = aabb_from_points(cpumesh->data.verts.data(), cpumesh->data.verts.size());

    // precompute modelspace BVH
    BVH_BuildFromTriangles(cpumesh->triangle_bvh, cpumesh->data.verts.data(), cpumesh->data.verts.size());

    return std::unique_ptr<Cached_meshdata>{new Cached_meshdata{cpumesh, upload_meshdata_for_instancing(cpumesh->data)}};
}

static void handle_line_emission(Simbody_geometry::Line const& l, Emitter_out& out) {
    Scene_decorations& dout = out.decs;

    Segment mesh_line{{0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    Segment emitted_line{l.p1, l.p2};

    glm::mat4x3 const& xform = dout.model_xforms.emplace_back(
        segment_to_segment_xform(mesh_line, emitted_line) * glm::scale(glm::mat4{1.0f}, {g_LineThickness * out.fixup_scale_factor, 1.0f, g_LineThickness * out.fixup_scale_factor}));
    dout.normal_xforms.emplace_back(normal_matrix(xform));
    dout.rgbas.emplace_back(rgba32_from_vec4(l.rgba));
    dout.gpu_meshes.emplace_back(out.cylinder.instance_meshdata);
    dout.cpu_meshes.emplace_back(out.cylinder.cpu_meshdata);
    dout.aabbs.emplace_back(aabb_apply_xform(out.cylinder.cpu_meshdata->aabb, xform));
    dout.components.emplace_back(out.c);
}

static void handle_cylinder_emission(Simbody_geometry::Cylinder const& cy, Emitter_out& out) {
    Scene_decorations& dout = out.decs;

    glm::mat4x3 const& xform = dout.model_xforms.emplace_back(cy.model_mtx);
    dout.normal_xforms.push_back(normal_matrix(xform));
    dout.rgbas.push_back(rgba32_from_vec4(cy.rgba));
    dout.gpu_meshes.push_back(out.cylinder.instance_meshdata);
    dout.cpu_meshes.push_back(out.cylinder.cpu_meshdata);
    dout.aabbs.push_back(aabb_apply_xform(out.cylinder.cpu_meshdata->aabb, xform));
    dout.components.push_back(out.c);
}

static void handle_cone_emission(Simbody_geometry::Cone const& cone, Emitter_out& out) {
    Scene_decorations& dout = out.decs;

    Segment meshline{{0.0f, -1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}};
    Segment coneline{cone.pos, cone.pos + cone.direction*cone.height};
    glm::mat4 line_xform = segment_to_segment_xform(meshline, coneline);
    glm::mat4 radius_rescale = glm::scale(glm::mat4{1.0f}, {cone.base_radius, 1.0f, cone.base_radius});

    glm::mat4x3 const& xform = dout.model_xforms.emplace_back(line_xform * radius_rescale);
    dout.normal_xforms.push_back(normal_matrix(xform));
    dout.rgbas.push_back(rgba32_from_vec4(cone.rgba));
    dout.gpu_meshes.push_back(out.cone.instance_meshdata);
    dout.cpu_meshes.push_back(out.cone.cpu_meshdata);
    dout.aabbs.push_back(aabb_apply_xform(out.cone.cpu_meshdata->aabb, xform));
    dout.components.push_back(out.c);
}

static void handle_sphere_emission(Simbody_geometry::Sphere const& s, Emitter_out& out) {
    Scene_decorations& sd = out.decs;

    // this code is fairly custom to make it faster
    //
    // - OpenSim scenes typically contain *a lot* of spheres
    // - it's much cheaper to compute things like normal matrices and AABBs when
    //   you know it's a sphere
    float scaled_r = out.fixup_scale_factor * s.radius;
    glm::mat4 xform;
    xform[0] = {scaled_r, 0.0f, 0.0f, 0.0f};
    xform[1] = {0.0f, scaled_r, 0.0f, 0.0f};
    xform[2] = {0.0f, 0.0f, scaled_r, 0.0f};
    xform[3] = glm::vec4{s.pos, 1.0f};
    glm::mat4 normal_xform = glm::transpose(xform);
    AABB aabb = sphere_aabb(Sphere{s.pos, scaled_r});

    sd.model_xforms.emplace_back(xform);
    sd.normal_xforms.emplace_back(normal_xform);
    sd.rgbas.emplace_back(rgba32_from_vec4(s.rgba));
    sd.gpu_meshes.emplace_back(out.sphere.instance_meshdata);
    sd.cpu_meshes.emplace_back(out.sphere.cpu_meshdata);
    sd.aabbs.emplace_back(aabb);
    sd.components.push_back(out.c);
}

static void handle_brick_emission(Simbody_geometry::Brick const& b, Emitter_out& out) {
    Scene_decorations& dout = out.decs;

    glm::mat4x3 const& xform = dout.model_xforms.emplace_back(b.model_mtx);
    dout.normal_xforms.push_back(normal_matrix(xform));
    dout.rgbas.push_back(rgba32_from_vec4(b.rgba));
    dout.gpu_meshes.push_back(out.brick.instance_meshdata);
    dout.cpu_meshes.push_back(out.brick.cpu_meshdata);
    dout.aabbs.push_back(aabb_apply_xform(out.brick.cpu_meshdata->aabb, xform));
    dout.components.push_back(out.c);
}

static void handle_meshfile_emission(Simbody_geometry::MeshFile const& mf, Emitter_out& out) {

    auto [it, inserted] = out.mesh_cache.try_emplace(*mf.path, nullptr);

    if (inserted) {
        // mesh wasn't in the cache, go load it
        try {
            it->second = create_cached_meshdata(stk_load_mesh(*mf.path));
        } catch (...) {
            // problems loading it: ensure cache isn't corrupted with the nullptr
            out.mesh_cache.erase(it);
            throw;
        }
    }

    // not null, because of the above insertion check
    Cached_meshdata& cm = *it->second;

    Scene_decorations& sd = out.decs;

    glm::mat4x3 const& xform = sd.model_xforms.emplace_back(mf.model_mtx);
    sd.normal_xforms.emplace_back(normal_matrix(xform));
    sd.rgbas.emplace_back(rgba32_from_vec4(mf.rgba));
    sd.gpu_meshes.emplace_back(cm.instance_meshdata);
    sd.cpu_meshes.emplace_back(cm.cpu_meshdata);
    sd.aabbs.emplace_back(aabb_apply_xform(cm.cpu_meshdata->aabb, xform));
    sd.components.emplace_back(out.c);
}

static void handle_frame_emission(Simbody_geometry::Frame const& frame, Emitter_out& out) {
    Scene_decorations& dout = out.decs;

    // emit origin sphere
    {
        Sphere mesh_sphere{{0.0f, 0.0f, 0.0f}, 1.0f};
        Sphere output_sphere{frame.pos, 0.05f * g_FrameAxisLengthRescale * out.fixup_scale_factor};

        glm::mat4x3 const& xform = dout.model_xforms.emplace_back(sphere_to_sphere_xform(mesh_sphere, output_sphere));
        dout.normal_xforms.push_back(normal_matrix(xform));
        dout.rgbas.push_back(rgba32_from_u32(0xffffffff));
        dout.gpu_meshes.push_back(out.sphere.instance_meshdata);
        dout.cpu_meshes.push_back(out.sphere.cpu_meshdata);
        dout.aabbs.push_back(aabb_apply_xform(out.sphere.cpu_meshdata->aabb, xform));
        dout.components.push_back(out.c);
    }

    // emit axis cylinders
    Segment cylinderline{{0.0f, -1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}};
    for (int i = 0; i < 3; ++i) {
        glm::vec3 dir = {0.0f, 0.0f, 0.0f};
        dir[i] = g_FrameAxisLengthRescale * out.fixup_scale_factor * frame.axis_lengths[i];
        Segment axisline{frame.pos, frame.pos + dir};

        glm::vec3 prescale = {g_FrameAxisThickness * out.fixup_scale_factor, 1.0f, g_FrameAxisThickness * out.fixup_scale_factor};
        glm::mat4 prescale_mtx = glm::scale(glm::mat4{1.0f}, prescale);
        glm::vec4 color{0.0f, 0.0f, 0.0f, 1.0f};
        color[i] = 1.0f;

        glm::mat4x3 const& xform = dout.model_xforms.emplace_back(segment_to_segment_xform(cylinderline, axisline) * prescale_mtx);
        dout.normal_xforms.push_back(normal_matrix(xform));
        dout.rgbas.push_back(rgba32_from_vec4(color));
        dout.gpu_meshes.push_back(out.cylinder.instance_meshdata);
        dout.cpu_meshes.push_back(out.cylinder.cpu_meshdata);
        dout.aabbs.push_back(aabb_apply_xform(out.cylinder.cpu_meshdata->aabb, xform));
        dout.components.push_back(out.c);
    }
}

static void handle_ellipsoid_emission(Simbody_geometry::Ellipsoid const& elip, Emitter_out& out) {
    Scene_decorations& sd = out.decs;

    glm::mat4x3 const& xform = sd.model_xforms.emplace_back(elip.model_mtx);
    sd.normal_xforms.push_back(normal_matrix(xform));
    sd.rgbas.push_back(rgba32_from_vec4(elip.rgba));
    sd.gpu_meshes.push_back(out.sphere.instance_meshdata);
    sd.cpu_meshes.push_back(out.sphere.cpu_meshdata);
    sd.aabbs.push_back(aabb_apply_xform(out.sphere.cpu_meshdata->aabb, xform));
    sd.components.push_back(out.c);
}

static void handle_arrow_emission(Simbody_geometry::Arrow const& a, Emitter_out& out) {
    Scene_decorations& dout = out.decs;

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
            dout.model_xforms.emplace_back(segment_to_segment_xform(meshline, Segment{cone_start, cone_end}) * cone_radius_rescaler);
        dout.normal_xforms.push_back(normal_matrix(xform));
        dout.rgbas.push_back(rgba32_from_vec4(a.rgba));
        dout.gpu_meshes.push_back(out.cone.instance_meshdata);
        dout.cpu_meshes.push_back(out.cone.cpu_meshdata);
        dout.aabbs.push_back(aabb_apply_xform(out.cone.cpu_meshdata->aabb, xform));
        dout.components.push_back(out.c);
    }

    // emit arrow's tail (a cylinder)
    {
        glm::mat4 cylinder_radius_rescaler = glm::scale(glm::mat4{1.0f}, {0.005f, 1.0f, 0.005f});
        glm::mat4x3 const& xform =
            dout.model_xforms.emplace_back(segment_to_segment_xform(meshline, Segment{cylinder_start, cone_start}) * cylinder_radius_rescaler);
        dout.normal_xforms.push_back(xform);
        dout.rgbas.push_back(rgba32_from_vec4(a.rgba));
        dout.gpu_meshes.push_back(out.cylinder.instance_meshdata);
        dout.cpu_meshes.push_back(out.cylinder.cpu_meshdata);
        dout.aabbs.push_back(aabb_apply_xform(out.cylinder.cpu_meshdata->aabb, xform));
        dout.components.push_back(out.c);
    }
}

// this is effectively called whenever OpenSim emits a decoration element
static void handle_geometry_emission(Simbody_geometry const& g, Emitter_out& out) {
    switch (g.geom_type) {
    case Simbody_geometry::Type::Sphere:
        handle_sphere_emission(g.sphere, out);
        break;
    case Simbody_geometry::Type::Line:
        handle_line_emission(g.line, out);
        break;
    case Simbody_geometry::Type::Cylinder:
        handle_cylinder_emission(g.cylinder, out);
        break;
    case Simbody_geometry::Type::Brick:
        handle_brick_emission(g.brick, out);
        break;
    case Simbody_geometry::Type::MeshFile:
        handle_meshfile_emission(g.meshfile, out);
        break;
    case Simbody_geometry::Type::Frame:
        handle_frame_emission(g.frame, out);
        break;
    case Simbody_geometry::Type::Ellipsoid:
        handle_ellipsoid_emission(g.ellipsoid, out);
        break;
    case Simbody_geometry::Type::Cone:
        handle_cone_emission(g.cone, out);
        break;
    case Simbody_geometry::Type::Arrow:
        handle_arrow_emission(g.arrow, out);
        break;
    }
}

void osc::Scene_decorations::clear() {
    model_xforms.clear();
    normal_xforms.clear();
    rgbas.clear();
    gpu_meshes.clear();
    cpu_meshes.clear();
    aabbs.clear();
    components.clear();
    aabb_bvh.clear();
}

osc::Scene_generator::Scene_generator() {
    m_CachedMeshes[g_SphereID] = create_cached_meshdata(gen_untextured_uv_sphere(12, 12));
    m_CachedMeshes[g_CylinderID] = create_cached_meshdata(gen_untextured_simbody_cylinder(16));
    m_CachedMeshes[g_BrickID] = create_cached_meshdata(gen_cube());
    m_CachedMeshes[g_ConeID] = create_cached_meshdata(gen_untextured_simbody_cone(12));
}

void osc::Scene_generator::generate(
        OpenSim::Component const& c,
        SimTK::State const& state,
        OpenSim::ModelDisplayHints const& hints,
        Modelstate_decoration_generator_flags flags,
        float fixup_scale_factor,
        Scene_decorations& out) {

    m_GeomListCache.clear();
    out.clear();

    // this is passed around a lot during emission - it's what each geometry emitter
    // uses to find/write things
    Emitter_out eo{
        m_CachedMeshes,
        *m_CachedMeshes[g_SphereID],
        *m_CachedMeshes[g_CylinderID],
        *m_CachedMeshes[g_BrickID],
        *m_CachedMeshes[g_ConeID],
        nullptr,
        out,
        fixup_scale_factor,
    };

    // called whenever OpenSim emits geometry
    auto on_geometry_emission = [&eo](Simbody_geometry const& g) {
        handle_geometry_emission(g, eo);
    };

    // get component's matter subsystem
    SimTK::SimbodyMatterSubsystem const& matter = c.getSystem().getMatterSubsystem();

    // create a visitor that visits each component
    auto visitor = Geometry_generator_lambda{matter, state, on_geometry_emission};

    // iterate through each component and walk through the geometry
    for (OpenSim::Component const& c : c.getComponentList()) {
        eo.c = &c;

        // emit static geometry (if requested)
        if (flags & Modelstate_decoration_generator_flags_GenerateStaticDecorations) {
            c.generateDecorations(true, hints, state, m_GeomListCache);
            for (SimTK::DecorativeGeometry const& dg : m_GeomListCache) {
                dg.implementGeometry(visitor);
            }
            m_GeomListCache.clear();
        }

        // emit dynamic geometry (if requested)
        if (flags & Modelstate_decoration_generator_flags_GenerateDynamicDecorations) {
            c.generateDecorations(false, hints, state, m_GeomListCache);
            for (SimTK::DecorativeGeometry const& dg : m_GeomListCache) {
                dg.implementGeometry(visitor);
            }
            m_GeomListCache.clear();
        }
    }

    OSC_ASSERT_ALWAYS(out.model_xforms.size() == out.normal_xforms.size());
    OSC_ASSERT_ALWAYS(out.normal_xforms.size() == out.rgbas.size());
    OSC_ASSERT_ALWAYS(out.rgbas.size() == out.gpu_meshes.size());
    OSC_ASSERT_ALWAYS(out.gpu_meshes.size() == out.cpu_meshes.size());
    OSC_ASSERT_ALWAYS(out.cpu_meshes.size() == out.aabbs.size());

    // the geometry pass above populates everything but the scene BVH via the lambda
    BVH_BuildFromAABBs(out.aabb_bvh, out.aabbs.data(), out.aabbs.size());
}

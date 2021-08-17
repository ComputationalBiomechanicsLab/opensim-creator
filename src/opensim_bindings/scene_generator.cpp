#include "scene_generator.hpp"

#include "src/assertions.hpp"
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

// these geometries are always added by the scene generator into the meshdata list
// in this order, so  that the implementation doesn't need to perform any hashtable
// lookups for them at runtime (OpenSim models can emit a large number of spheres +
// cylinders)
static constexpr unsigned short g_SphereMeshidx = 0;
static constexpr unsigned short g_CylinderMeshidx = 1;
static constexpr unsigned short g_BrickMeshidx = 2;
static constexpr unsigned short g_ConeMeshidx = 3;

static constexpr float g_LineThickness = 0.005f;
static constexpr float g_FrameAxisLengthRescale = 0.25f;
static constexpr float g_FrameAxisThickness = 0.0025f;

// preprocess (AABB, BVH, etc.) CPU-side data and upload it to the instanced renderer
static Cached_meshdata create_cached_meshdata(Instanced_renderer& renderer, NewMesh src_mesh) {

    // heap-allocate the meshdata into a shared pointer so that emitted drawlists can
    // independently contain a (reference-counted) pointer to the data
    std::shared_ptr<CPU_mesh> cpumesh = std::make_shared<CPU_mesh>();
    cpumesh->data = std::move(src_mesh);

    // precompute modelspace AABB
    cpumesh->aabb = aabb_from_points(cpumesh->data.verts.data(), cpumesh->data.verts.size());

    // precompute modelspace BVH
    BVH_BuildFromTriangles(cpumesh->triangle_bvh, cpumesh->data.verts.data(), cpumesh->data.verts.size());

    return Cached_meshdata{cpumesh, renderer.allocate(cpumesh->data)};
}

static void handle_line_emission(
        OpenSim::Component const* c,
        Simbody_geometry::Line const& l,
        Scene_decorations& out) {
    Segment mesh_line{{0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    Segment emitted_line{l.p1, l.p2};

    // emit instance data
    short data = static_cast<short>(out.drawlist.instances.size());
    Mesh_instance& ins = out.drawlist.instances.emplace_back();
    ins.model_xform = segment_to_segment_xform(mesh_line, emitted_line) * glm::scale(glm::mat4{1.0f}, {g_LineThickness, 1.0f, g_LineThickness});
    ins.normal_xform = normal_matrix(ins.model_xform);
    ins.rgba = rgba32_from_vec4(l.rgba);
    ins.meshidx = g_CylinderMeshidx;
    ins.texidx = -1;
    ins.rim_intensity = 0x00;
    ins.data = data;

    // emit worldspace aabb for the instance
    AABB aabb = aabb_apply_xform(out.meshes_data[g_CylinderMeshidx]->aabb, ins.model_xform);

    out.aabbs.push_back(aabb);
    out.meshidxs.push_back(ins.meshidx);
    out.model_mtxs.push_back(ins.model_xform);
    out.components.push_back(c);
}

static void handle_sphere_emission(
        OpenSim::Component const* c,
        Simbody_geometry::Sphere const& s,
        Scene_decorations& out) {

    glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, glm::vec3{s.radius, s.radius, s.radius});
    glm::mat4 mover = glm::translate(glm::mat4{1.0f}, s.pos);

    // emit instance data
    short data = static_cast<short>(out.drawlist.instances.size());
    Mesh_instance& ins = out.drawlist.instances.emplace_back();
    ins.model_xform = mover * scaler;
    ins.normal_xform = normal_matrix(ins.model_xform);
    ins.rgba = rgba32_from_vec4(s.rgba);
    ins.meshidx = g_SphereMeshidx;
    ins.texidx = -1;
    ins.rim_intensity = 0x00;
    ins.data = data;

    // emit worldspace aabb for the instance
    AABB aabb = aabb_apply_xform(out.meshes_data[g_SphereMeshidx]->aabb, ins.model_xform);
    out.aabbs.push_back(aabb);
    out.meshidxs.push_back(ins.meshidx);
    out.model_mtxs.push_back(ins.model_xform);
    out.components.push_back(c);
}

static void handle_cylinder_emission(
        OpenSim::Component const* c,
        Simbody_geometry::Cylinder const& cy,
        Scene_decorations& out) {

    // emit instance data
    short data = static_cast<short>(out.drawlist.instances.size());
    Mesh_instance& ins = out.drawlist.instances.emplace_back();
    ins.model_xform = cy.model_mtx;
    ins.normal_xform = normal_matrix(ins.model_xform);
    ins.rgba = rgba32_from_vec4(cy.rgba);
    ins.meshidx = g_CylinderMeshidx;
    ins.texidx = -1;
    ins.rim_intensity = 0x00;
    ins.data = data;

    // emit worldspace aabb for the instance
    AABB aabb = aabb_apply_xform(out.meshes_data[g_CylinderMeshidx]->aabb, ins.model_xform);
    out.aabbs.push_back(aabb);
    out.meshidxs.push_back(ins.meshidx);
    out.model_mtxs.push_back(ins.model_xform);
    out.components.push_back(c);
}

static void handle_brick_emission(
        OpenSim::Component const* c,
        Simbody_geometry::Brick const& b,
        Scene_decorations& out) {

    // emit instance data
    short data = static_cast<short>(out.drawlist.instances.size());
    Mesh_instance& ins = out.drawlist.instances.emplace_back();
    ins.model_xform = b.model_mtx;
    ins.normal_xform = normal_matrix(ins.model_xform);
    ins.rgba = rgba32_from_vec4(b.rgba);
    ins.meshidx = g_BrickMeshidx;
    ins.texidx = -1;
    ins.rim_intensity = 0x00;
    ins.data = data;

    // emit worldspace aabb for the instance
    AABB aabb = aabb_apply_xform(out.meshes_data[g_BrickMeshidx]->aabb, ins.model_xform);
    out.aabbs.push_back(aabb);
    out.meshidxs.push_back(ins.meshidx);
    out.model_mtxs.push_back(ins.model_xform);
    out.components.push_back(c);
}

static void handle_meshfile_emission(
        std::unordered_map<std::string, std::unique_ptr<Cached_meshdata>>& mesh_cache,
        std::unordered_map<Cached_meshdata*, int>& meshdata2idx,
        Instanced_renderer& r,
        OpenSim::Component const* c,
        Simbody_geometry::MeshFile const& mf,
        Scene_decorations& out) {

    auto [it, inserted] = mesh_cache.try_emplace(*mf.path, nullptr);

    if (inserted) {
        // mesh wasn't in the cache, go load it
        try {
            it->second = std::make_unique<Cached_meshdata>(create_cached_meshdata(r, stk_load_mesh(*mf.path)));
        } catch (...) {
            // problems loading it: ensure cache isn't corrupted with the nullptr
            mesh_cache.erase(it);
            throw;
        }
    }

    // not null, because of the above insertion check
    Cached_meshdata* cm = it->second.get();

    // lookup the (assumed) index of the mesh in the existing drawlist
    auto [it2, inserted_new_meshidx] = meshdata2idx.try_emplace(cm, -1);
    if (inserted_new_meshidx) {
        OSC_ASSERT(out.drawlist.meshes.size() == out.meshes_data.size());

        // it isn't in the meshdata section of the output yet, so go allocate it
        unsigned short meshidx = static_cast<unsigned short>(out.drawlist.meshes.size());
        out.drawlist.meshes.push_back(cm->instance_meshdata);
        out.meshes_data.push_back(cm->cpu_meshdata);

        it2->second = meshidx;
    }

    // this is safe because of the above insertion check
    unsigned short meshidx = it2->second;

    // emit instance data
    short data = static_cast<short>(out.drawlist.instances.size());
    Mesh_instance& ins = out.drawlist.instances.emplace_back();
    ins.model_xform = mf.model_mtx;
    ins.normal_xform = normal_matrix(ins.model_xform);
    ins.rgba = rgba32_from_vec4(mf.rgba);
    ins.meshidx = meshidx;
    ins.texidx = -1;
    ins.rim_intensity = 0x00;
    ins.data = data;

    // emit worldspace aabb for the instance
    AABB aabb = aabb_apply_xform(out.meshes_data[meshidx]->aabb, ins.model_xform);
    out.aabbs.push_back(aabb);
    out.meshidxs.push_back(ins.meshidx);
    out.model_mtxs.push_back(ins.model_xform);
    out.components.push_back(c);
}

static void handle_cone_emission(
        OpenSim::Component const* c,
        Simbody_geometry::Cone const& cone,
        Scene_decorations& out) {

    Segment meshline{{0.0f, -1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}};
    Segment coneline{cone.pos, cone.pos + cone.direction*cone.height};
    glm::mat4 xform = segment_to_segment_xform(meshline, coneline);
    glm::mat4 radius_rescale = glm::scale(glm::mat4{1.0f}, {cone.base_radius, 1.0f, cone.base_radius});

    // emit instance data
    short data = static_cast<short>(out.drawlist.instances.size());
    Mesh_instance& ins = out.drawlist.instances.emplace_back();
    ins.model_xform =  xform * radius_rescale;
    ins.normal_xform = normal_matrix(ins.model_xform);
    ins.rgba = rgba32_from_vec4(cone.rgba);
    ins.meshidx = g_ConeMeshidx;
    ins.texidx = -1;
    ins.rim_intensity = 0x00;
    ins.data = data;

    // emit worldspace aabb for the instance
    AABB aabb = aabb_apply_xform(out.meshes_data[g_ConeMeshidx]->aabb, ins.model_xform);
    out.aabbs.push_back(aabb);
    out.meshidxs.push_back(ins.meshidx);
    out.model_mtxs.push_back(ins.model_xform);
    out.components.push_back(c);
}

static void handle_frame_emission(
        OpenSim::Component const* c,
        Simbody_geometry::Frame const& frame,
        Scene_decorations& out) {

    // generate origin sphere
    {
        Sphere mesh_sphere{{0.0f, 0.0f, 0.0f}, 1.0f};
        Sphere output_sphere{frame.pos, 0.05f * g_FrameAxisLengthRescale};

        // emit instance data
        short data = static_cast<short>(out.drawlist.instances.size());
        Mesh_instance& ins = out.drawlist.instances.emplace_back();
        ins.model_xform = sphere_to_sphere_xform(mesh_sphere, output_sphere);
        ins.normal_xform = normal_matrix(ins.model_xform);
        ins.rgba = rgba32_from_u32(0xffffffff);
        ins.meshidx = g_SphereMeshidx;
        ins.texidx = -1;
        ins.rim_intensity = 0x00;
        ins.data = data;

        // emit worldspace aabb for the instance
        AABB aabb = aabb_apply_xform(out.meshes_data[g_SphereMeshidx]->aabb, ins.model_xform);
        out.aabbs.push_back(aabb);
        out.meshidxs.push_back(ins.meshidx);
        out.model_mtxs.push_back(ins.model_xform);
        out.components.push_back(c);
    }

    // generate axis cylinders
    Segment cylinderline{{0.0f, -1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}};
    for (int i = 0; i < 3; ++i) {
        glm::vec3 dir = {0.0f, 0.0f, 0.0f};
        dir[i] = g_FrameAxisLengthRescale * frame.axis_lengths[i];
        Segment axisline{frame.pos, frame.pos + dir};

        glm::vec3 prescale = {g_FrameAxisThickness, 1.0f, g_FrameAxisThickness};
        glm::mat4 prescale_mtx = glm::scale(glm::mat4{1.0f}, prescale);
        glm::vec4 color{0.0f, 0.0f, 0.0f, 1.0f};
        color[i] = 1.0f;

        glm::mat4 xform = segment_to_segment_xform(cylinderline, axisline);

        short data = static_cast<short>(out.drawlist.instances.size());
        Mesh_instance& ins = out.drawlist.instances.emplace_back();
        ins.model_xform = xform * prescale_mtx;
        ins.normal_xform = normal_matrix(ins.model_xform);
        ins.rgba = rgba32_from_vec4(color);
        ins.meshidx = g_CylinderMeshidx;
        ins.texidx = -1;
        ins.rim_intensity = 0x00;
        ins.data = data;

        // emit worldspace aabb for the instance
        AABB aabb = aabb_apply_xform(out.meshes_data[g_CylinderMeshidx]->aabb, ins.model_xform);
        out.aabbs.push_back(aabb);
        out.meshidxs.push_back(ins.meshidx);
        out.model_mtxs.push_back(ins.model_xform);
        out.components.push_back(c);
    }
}

// this is effectively called whenever OpenSim emits a decoration element
static void handle_geometry_emission(
        std::unordered_map<std::string, std::unique_ptr<Cached_meshdata>>& mesh_cache,
        std::unordered_map<Cached_meshdata*, int>& meshdata2idx,
        Instanced_renderer& r,
        OpenSim::Component const* c,
        Simbody_geometry const& g,
        Scene_decorations& out) {

    switch (g.geom_type) {
    case Simbody_geometry::Type::Sphere:
        handle_sphere_emission(c, g.sphere, out);
        break;
    case Simbody_geometry::Type::Line:
        handle_line_emission(c, g.line, out);
        break;
    case Simbody_geometry::Type::Cylinder:
        handle_cylinder_emission(c, g.cylinder, out);
        break;
    case Simbody_geometry::Type::Brick:
        handle_brick_emission(c, g.brick, out);
        break;
    case Simbody_geometry::Type::MeshFile:
        handle_meshfile_emission(mesh_cache, meshdata2idx, r, c, g.meshfile, out);
        break;
    case Simbody_geometry::Type::Frame:
        handle_frame_emission(c, g.frame, out);
        break;
    case Simbody_geometry::Type::Ellipsoid:
        // TODO
        break;
    case Simbody_geometry::Type::Cone:
        handle_cone_emission(c, g.cone, out);
        break;
    case Simbody_geometry::Type::Arrow:
        // TODO
        break;
    }
}

void osc::Scene_decorations::clear() {
    drawlist.clear();
    meshes_data.clear();
    aabbs.clear();
    aabb_bvh.clear();
    components.clear();
    meshidxs.clear();
    model_mtxs.clear();
}

osc::Scene_generator::Scene_generator(Instanced_renderer& r) :
    m_CachedSphere{create_cached_meshdata(r, gen_untextured_uv_sphere(12, 12))},
    m_CachedCylinder{create_cached_meshdata(r, gen_untextured_simbody_cylinder(16))},
    m_CachedBrick{create_cached_meshdata(r, gen_cube())},
    m_CachedCone{create_cached_meshdata(r, gen_untextured_simbody_cone(12))},
    m_CachedMeshes{} {
}

void osc::Scene_generator::generate(
        Instanced_renderer& r,
        OpenSim::Component const& c,
        SimTK::State const& state,
        OpenSim::ModelDisplayHints const& hints,
        Scene_decorations& out,
        Modelstate_decoration_generator_flags flags) {

    // clear the pointer-to-meshidx cache that's used to dedupe mesh references
    m_MeshPtr2Meshidx.clear();

    // clear this - it's required by OpenSim's generatedecorations thing
    m_GeomListCache.clear();

    // clear any existing data in the drawlist
    out.clear();

    // preallocate common meshes, so that we don't have to do redundant
    // hashtable lookups (models can contain *a lot* of spheres+cylinders)
    //
    // CARE: these need to match the globals
    out.drawlist.meshes.push_back(m_CachedSphere.instance_meshdata);
    out.meshes_data.push_back(m_CachedSphere.cpu_meshdata);
    out.drawlist.meshes.push_back(m_CachedCylinder.instance_meshdata);
    out.meshes_data.push_back(m_CachedCylinder.cpu_meshdata);
    out.drawlist.meshes.push_back(m_CachedBrick.instance_meshdata);
    out.meshes_data.push_back(m_CachedBrick.cpu_meshdata);
    out.drawlist.meshes.push_back(m_CachedCone.instance_meshdata);
    out.meshes_data.push_back(m_CachedCone.cpu_meshdata);

    // called whenever the simbody geometry generator emits new geometry
    OpenSim::Component const* current_component = nullptr;
    auto on_geometry_emission = [&r, &current_component, &out, this](Simbody_geometry const& g) {
        handle_geometry_emission(m_CachedMeshes, m_MeshPtr2Meshidx, r, current_component, g, out);
    };
    SimTK::SimbodyMatterSubsystem const& matter = c.getSystem().getMatterSubsystem();

    // create a visitor that visits each component
    auto visitor = Geometry_generator_lambda{matter, state, on_geometry_emission};

    // iterate through each component and walk through the geometry
    for (OpenSim::Component const& c : c.getComponentList()) {
        current_component = &c;

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

    // the geometry pass above populates everything but the scene BVH via the lambda
    BVH_BuildFromAABBs(out.aabb_bvh, out.aabbs.data(), out.aabbs.size());
}

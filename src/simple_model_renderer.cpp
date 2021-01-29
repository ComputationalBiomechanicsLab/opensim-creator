#include "simple_model_renderer.hpp"

#include "raw_renderer.hpp"

#include "3d_common.hpp"
#include "application.hpp"
#include "cfg.hpp"
#include "gl.hpp"
#include "opensim_wrapper.hpp"
#include "sdl_wrapper.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <OpenSim/OpenSim.h>
#include <glm/gtc/matrix_transform.hpp>

#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <vector>

namespace {
    class Application;

    // this is global-ed because renderers + meshes might be duped between
    // the various screens in OSMV and it's efficient to have everything
    // freewheeled
    struct Global_opensim_mesh_loader_state final {
        // reserved mesh IDs:
        //
        // these are meshes that aren't actually loaded from a file, but generated. Things like
        // spheres and planes fall into this category. They are typically generated on the CPU
        // once and then uploaded onto the GPU. Then, whenever OpenSim/Simbody want one they can
        // just use the meshid to automatically freewheel it from the GPU.
        int sphere_meshid;
        int cylinder_meshid;
        int cube_meshid;

        // path-to-meshid lookup
        //
        // allows decoration generators to lookup whether a mesh file (e.g. pelvis.vtp)
        // has already been uploaded to the GPU or not and, if it has, what meshid it
        // was assigned
        //
        // this is necessary because SimTK will emit mesh information as paths on the
        // filesystem
        std::unordered_map<std::string, int> path2meshid;

        // swap space for Simbody's generateDecorations append target
        //
        // generateDecorations requires an Array_ outparam
        SimTK::Array_<SimTK::DecorativeGeometry> dg_swap;

        // swap space for osmv::Untextured_vert
        //
        // this is generally the format needed for GPU uploads
        std::vector<osmv::Untextured_vert> vert_swap;

        Global_opensim_mesh_loader_state() {
            osmv::unit_sphere_triangles(vert_swap);
            sphere_meshid = osmv::globally_allocate_mesh(vert_swap.data(), vert_swap.size());

            osmv::simbody_cylinder_triangles(vert_swap);
            cylinder_meshid = osmv::globally_allocate_mesh(vert_swap.data(), vert_swap.size());

            osmv::simbody_brick_triangles(vert_swap);
            cube_meshid = osmv::globally_allocate_mesh(vert_swap.data(), vert_swap.size());
        }
    };

    // getter for the global mesh loader instance
    //
    // must only be called after OpenGL is initialized.
    Global_opensim_mesh_loader_state& global_meshes() {
        static Global_opensim_mesh_loader_state data;
        return data;
    }
}

using namespace SimTK;
using namespace OpenSim;

// OpenSim rendering specifics
namespace {
    // create an xform that transforms the unit cylinder into a line between
    // two points
    glm::mat4 cylinder_to_line_xform(float line_width, glm::vec3 const& p1, glm::vec3 const& p2) {
        glm::vec3 p1_to_p2 = p2 - p1;
        glm::vec3 c1_to_c2 = glm::vec3{0.0f, 2.0f, 0.0f};
        auto rotation = glm::rotate(
            glm::identity<glm::mat4>(),
            glm::acos(glm::dot(glm::normalize(c1_to_c2), glm::normalize(p1_to_p2))),
            glm::cross(glm::normalize(c1_to_c2), glm::normalize(p1_to_p2)));
        float scale = glm::length(p1_to_p2) / glm::length(c1_to_c2);
        auto scale_xform = glm::scale(glm::identity<glm::mat4>(), glm::vec3{line_width, scale, line_width});
        auto translation = glm::translate(glm::identity<glm::mat4>(), p1 + p1_to_p2 / 2.0f);
        return translation * rotation * scale_xform;
    }

    // load a SimTK::PolygonalMesh into an osmv::Untextured_vert mesh ready for GPU upload
    void load_mesh_data(PolygonalMesh const& mesh, std::vector<osmv::Untextured_vert>& triangles) {

        // helper function: gets a vertex for a face
        auto get_face_vert_pos = [&](int face, int vert) {
            SimTK::Vec3 pos = mesh.getVertexPosition(mesh.getFaceVertex(face, vert));
            return glm::vec3{pos[0], pos[1], pos[2]};
        };

        // helper function: compute the normal of the triangle p1, p2, p3
        auto make_normal = [](glm::vec3 const& p1, glm::vec3 const& p2, glm::vec3 const& p3) {
            return glm::cross(p2 - p1, p3 - p1);
        };

        triangles.clear();

        // iterate over each face in the PolygonalMesh and transform each into a sequence of
        // GPU-friendly triangle verts
        for (auto face = 0; face < mesh.getNumFaces(); ++face) {
            auto num_vertices = mesh.getNumVerticesForFace(face);

            if (num_vertices < 3) {
                // line?: ignore for now
            } else if (num_vertices == 3) {
                // triangle: use as-is
                glm::vec3 p1 = get_face_vert_pos(face, 0);
                glm::vec3 p2 = get_face_vert_pos(face, 1);
                glm::vec3 p3 = get_face_vert_pos(face, 2);
                glm::vec3 normal = make_normal(p1, p2, p3);

                triangles.push_back({p1, normal});
                triangles.push_back({p2, normal});
                triangles.push_back({p3, normal});
            } else if (num_vertices == 4) {
                // quad: split into two triangles

                glm::vec3 p1 = get_face_vert_pos(face, 0);
                glm::vec3 p2 = get_face_vert_pos(face, 1);
                glm::vec3 p3 = get_face_vert_pos(face, 2);
                glm::vec3 p4 = get_face_vert_pos(face, 3);

                glm::vec3 t1_norm = make_normal(p1, p2, p3);
                glm::vec3 t2_norm = make_normal(p3, p4, p1);

                triangles.push_back({p1, t1_norm});
                triangles.push_back({p2, t1_norm});
                triangles.push_back({p3, t1_norm});

                triangles.push_back({p3, t2_norm});
                triangles.push_back({p4, t2_norm});
                triangles.push_back({p1, t2_norm});
            } else {
                // polygon (>3 edges):
                //
                // create a vertex at the average center point and attach
                // every two verices to the center as triangles.

                auto center = glm::vec3{0.0f, 0.0f, 0.0f};
                for (int vert = 0; vert < num_vertices; ++vert) {
                    center += get_face_vert_pos(face, vert);
                }
                center /= num_vertices;

                for (int vert = 0; vert < num_vertices - 1; ++vert) {
                    glm::vec3 p1 = get_face_vert_pos(face, vert);
                    glm::vec3 p2 = get_face_vert_pos(face, vert + 1);
                    glm::vec3 normal = make_normal(p1, p2, center);

                    triangles.push_back({p1, normal});
                    triangles.push_back({p2, normal});
                    triangles.push_back({center, normal});
                }

                // complete the polygon loop
                glm::vec3 p1 = get_face_vert_pos(face, num_vertices - 1);
                glm::vec3 p2 = get_face_vert_pos(face, 0);
                glm::vec3 normal = make_normal(p1, p2, center);

                triangles.push_back({p1, normal});
                triangles.push_back({p2, normal});
                triangles.push_back({center, normal});
            }
        }
    }

    // a visitor that can be used with SimTK's `implementGeometry` method
    struct Geometry_visitor final : public DecorativeGeometryImplementation {
        SimbodyMatterSubsystem const& matter_subsystem;
        SimTK::State const& state;
        std::vector<osmv::Mesh_instance>& out;

        Geometry_visitor(
            SimbodyMatterSubsystem const& _matter_subsystem,
            SimTK::State const& _state,
            std::vector<osmv::Mesh_instance>& _out) :
            matter_subsystem{_matter_subsystem},
            state{_state},
            out{_out} {
        }

        Transform ground_to_decoration_xform(DecorativeGeometry const& geom) {
            SimbodyMatterSubsystem const& ms = matter_subsystem;
            MobilizedBody const& mobod = ms.getMobilizedBody(MobilizedBodyIndex(geom.getBodyId()));
            Transform const& ground_to_body_xform = mobod.getBodyTransform(state);
            Transform const& body_to_decoration_xform = geom.getTransform();

            return ground_to_body_xform * body_to_decoration_xform;
        }

        glm::mat4 transform(DecorativeGeometry const& geom) {
            Transform t = ground_to_decoration_xform(geom);
            glm::mat4 m = glm::identity<glm::mat4>();

            // glm::mat4 is column major:
            //     see: https://glm.g-truc.net/0.9.2/api/a00001.html
            //     (and just Google "glm column major?")
            //
            // SimTK is whoknowswtf-major (actually, row), carefully read the
            // sourcecode for `SimTK::Transform`.

            // x
            m[0][0] = static_cast<float>(t.R().row(0)[0]);
            m[0][1] = static_cast<float>(t.R().row(1)[0]);
            m[0][2] = static_cast<float>(t.R().row(2)[0]);
            m[0][3] = 0.0f;

            // y
            m[1][0] = static_cast<float>(t.R().row(0)[1]);
            m[1][1] = static_cast<float>(t.R().row(1)[1]);
            m[1][2] = static_cast<float>(t.R().row(2)[1]);
            m[1][3] = 0.0f;

            // z
            m[2][0] = static_cast<float>(t.R().row(0)[2]);
            m[2][1] = static_cast<float>(t.R().row(1)[2]);
            m[2][2] = static_cast<float>(t.R().row(2)[2]);
            m[2][3] = 0.0f;

            // w
            m[3][0] = static_cast<float>(t.p()[0]);
            m[3][1] = static_cast<float>(t.p()[1]);
            m[3][2] = static_cast<float>(t.p()[2]);
            m[3][3] = 1.0f;

            return m;
        }

        glm::vec3 scale_factors(DecorativeGeometry const& geom) {
            Vec3 sf = geom.getScaleFactors();
            for (int i = 0; i < 3; ++i) {
                sf[i] = sf[i] <= 0 ? 1.0 : sf[i];
            }
            return glm::vec3{sf[0], sf[1], sf[2]};
        }

        glm::vec4 rgba(DecorativeGeometry const& geom) {
            Vec3 const& rgb = geom.getColor();
            Real a = geom.getOpacity();
            return {rgb[0], rgb[1], rgb[2], a < 0.0f ? 1.0f : a};
        }

        glm::vec4 to_vec4(Vec3 const& v, float w = 1.0f) {
            return glm::vec4{v[0], v[1], v[2], w};
        }

        void implementPointGeometry(const DecorativePoint&) override {
            // nyi: should be implemented as a sphere as a quick hack (rather than GL_POINTS)
        }

        void implementLineGeometry(const DecorativeLine& geom) override {
            // a line is essentially a thin cylinder that connects two points
            // in space. This code eagerly performs that transformation

            glm::mat4 xform = transform(geom);
            glm::vec3 p1 = xform * to_vec4(geom.getPoint1());
            glm::vec3 p2 = xform * to_vec4(geom.getPoint2());

            glm::mat4 cylinder_xform = cylinder_to_line_xform(0.005f, p1, p2);

            out.emplace_back(cylinder_xform, rgba(geom), global_meshes().cylinder_meshid);
        }
        void implementBrickGeometry(const DecorativeBrick& geom) override {
            SimTK::Vec3 dims = geom.getHalfLengths();
            glm::mat4 xform = glm::scale(transform(geom), glm::vec3{dims[0], dims[1], dims[2]});

            out.emplace_back(xform, rgba(geom), global_meshes().cube_meshid);
        }
        void implementCylinderGeometry(const DecorativeCylinder& geom) override {
            glm::mat4 m = transform(geom);
            glm::vec3 s = scale_factors(geom);
            s.x *= static_cast<float>(geom.getRadius());
            s.y *= static_cast<float>(geom.getHalfHeight());
            s.z *= static_cast<float>(geom.getRadius());

            glm::mat4 xform = glm::scale(m, s);

            out.emplace_back(xform, rgba(geom), global_meshes().cylinder_meshid);
        }
        void implementCircleGeometry(const DecorativeCircle&) override {
            // nyi
        }
        void implementSphereGeometry(const DecorativeSphere& geom) override {
            float r = static_cast<float>(geom.getRadius());
            glm::mat4 xform = glm::scale(transform(geom), glm::vec3{r, r, r});

            out.emplace_back(xform, rgba(geom), global_meshes().sphere_meshid);
        }
        void implementEllipsoidGeometry(const DecorativeEllipsoid&) override {
            // nyi
        }
        void implementFrameGeometry(const DecorativeFrame&) override {
            // nyi
        }
        void implementTextGeometry(const DecorativeText&) override {
            // nyi
        }
        void implementMeshGeometry(const DecorativeMesh&) override {
            // nyi
        }
        void implementMeshFileGeometry(const DecorativeMeshFile& m) override {
            auto& global = global_meshes();

            // perform a cache search for the mesh
            int meshid = osmv::invalid_meshid;
            {
                auto [it, inserted] = global.path2meshid.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(std::ref(m.getMeshFile())),
                    std::forward_as_tuple(osmv::invalid_meshid));

                if (not inserted) {
                    // it wasn't inserted, so the path has already been loaded and the entry
                    // contains a meshid for the fully-loaded mesh
                    meshid = it->second;
                } else {
                    // it was inserted, and is currently a junk meshid because we haven't loaded
                    // a mesh yet. Load the mesh from the file onto the GPU and allocate a new
                    // meshid for it. Assign that meshid to the path2meshid lookup.
                    load_mesh_data(m.getMesh(), global.vert_swap);
                    meshid = osmv::globally_allocate_mesh(global.vert_swap.data(), global.vert_swap.size());
                    it->second = meshid;
                }
            }

            glm::mat4 xform = glm::scale(transform(m), scale_factors(m));
            out.emplace_back(xform, rgba(m), meshid);
        }
        void implementArrowGeometry(const DecorativeArrow&) override {
            // nyi
        }
        void implementTorusGeometry(const DecorativeTorus&) override {
            // nyi
        }
        void implementConeGeometry(const DecorativeCone&) override {
            // nyi
        }
    };

    glm::mat4 compute_view_matrix(float theta, float phi, float radius, glm::vec3 pan) {
        // camera: at a fixed position pointing at a fixed origin. The "camera"
        // works by translating + rotating all objects around that origin. Rotation
        // is expressed as polar coordinates. Camera panning is represented as a
        // translation vector.

        // this maths is a complete shitshow and I apologize. It just happens to work for now. It's a polar coordinate
        // system that shifts the world based on the camera pan

        auto rot_theta = glm::rotate(glm::identity<glm::mat4>(), -theta, glm::vec3{0.0f, 1.0f, 0.0f});
        auto theta_vec = glm::normalize(glm::vec3{sinf(theta), 0.0f, cosf(theta)});
        auto phi_axis = glm::cross(theta_vec, glm::vec3{0.0, 1.0f, 0.0f});
        auto rot_phi = glm::rotate(glm::identity<glm::mat4>(), -phi, phi_axis);
        auto pan_translate = glm::translate(glm::identity<glm::mat4>(), pan);
        return glm::lookAt(glm::vec3(0.0f, 0.0f, radius), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3{0.0f, 1.0f, 0.0f}) *
               rot_theta * rot_phi * pan_translate;
    }

    glm::vec3 spherical_2_cartesian(float theta, float phi, float radius) {
        return glm::vec3{radius * sinf(theta) * cosf(phi), radius * sinf(phi), radius * cosf(theta) * cosf(phi)};
    }

    // flags for the geometry generator
    using GeometryGeneratorFlags = int;
    enum GeometryGeneratorFlags_ {
        // here for completeness
        GeometryGeneratorFlags_None = 0,

        // only generate geometry for static decorations in the model
        GeometryGeneratorFlags_Static = 1 << 0,

        // only generate geometry for dynamic decorations in the model
        GeometryGeneratorFlags_Dynamic = 1 << 1,

        // default flags
        GeometryGeneratorFlags_Default = GeometryGeneratorFlags_Static | GeometryGeneratorFlags_Dynamic
    };

    // geometry + metadata pulled from an OpenSim model
    struct OpenSim_model_geometry final {
        std::vector<osmv::Mesh_instance> meshes;
        std::vector<OpenSim::Component const*> associated_components;

        void clear() {
            meshes.clear();
            associated_components.clear();
        }
    };

    void generate_geometry(
        OpenSim::Model const& model,
        SimTK::State const& st,
        OpenSim_model_geometry& append_out,
        GeometryGeneratorFlags flags = GeometryGeneratorFlags_Default) {

        // iterate over all components in the OpenSim model, keeping a few things in mind:
        //
        // - Anything in the component tree *might* render geometry
        //
        // - For selection logic, we only (currently) care about certain high-level components,
        //   like muscles
        //
        // - Pretend the component tree traversal is implementation-defined because OpenSim's
        //   implementation of component-tree walking is a bit of a clusterfuck. At time of
        //   writing, it's a breadth-first recursive descent
        //
        // - Components of interest, like muscles, might not render their geometry - it might be
        //   delegated to a subcomponent
        //
        // So this algorithm assumes that the list iterator is arbitrary, but always returns
        // *something* in a tree that has the current model as a root. So, for each component that
        // pops out of `getComponentList`, crawl "up" to the root. If we encounter something
        // interesting (e.g. a `Muscle`) then we tag the geometry against that component, rather
        // than the component that is rendering.

        // get a reusable swap-space for geometry generation
        SimTK::Array_<SimTK::DecorativeGeometry>& dg_swap = global_meshes().dg_swap;

        // create a visitor that is called by OpenSim whenever it wants to generate abstract
        // geometry
        Geometry_visitor visitor{model.getSystem().getMatterSubsystem(), st, append_out.meshes};
        OpenSim::ModelDisplayHints const& hints = model.getDisplayHints();

        for (OpenSim::Component const& c : model.getComponentList()) {
            // HACK: fixup the owners to be something more interesting
            OpenSim::Component const* owner = nullptr;
            for (OpenSim::Component const* p = &c; p != &model; p = &p->getOwner()) {
                if (dynamic_cast<OpenSim::Muscle const*>(p)) {
                    owner = p;
                    break;
                }
            }

            if (flags & GeometryGeneratorFlags_Static) {
                dg_swap.clear();
                c.generateDecorations(true, hints, st, dg_swap);

                // static geometry has no "owner"
                for (size_t i = 0; i < dg_swap.size(); ++i) {
                    append_out.associated_components.push_back(nullptr);
                }

                // populate append_out.meshes
                for (SimTK::DecorativeGeometry const& geom : dg_swap) {
                    geom.implementGeometry(visitor);
                }

                assert(append_out.meshes.size() == append_out.associated_components.size());
            }

            if (flags & GeometryGeneratorFlags_Dynamic) {
                dg_swap.clear();
                c.generateDecorations(false, hints, st, dg_swap);

                // assign owner
                for (size_t i = 0; i < dg_swap.size(); ++i) {
                    append_out.associated_components.push_back(owner);
                }

                // populate append_out.meshes
                for (SimTK::DecorativeGeometry const& geom : dg_swap) {
                    geom.implementGeometry(visitor);
                }

                assert(append_out.meshes.size() == append_out.associated_components.size());
            }
        }
    }
}

namespace osmv {
    struct Simple_model_renderer_impl final {
        Raw_renderer renderer;
        OpenSim_model_geometry geom_swap;

        Simple_model_renderer_impl(int w, int h, int samples) : renderer{w, h, samples} {
        }
    };
}

osmv::Simple_model_renderer::Simple_model_renderer(int w, int h, int samples) :
    impl(new Simple_model_renderer_impl(w, h, samples)) {
}

osmv::Simple_model_renderer::~Simple_model_renderer() noexcept {
    delete impl;
}

bool osmv::Simple_model_renderer::on_event(Application& app, SDL_Event const& e) {
    // edge-case: the event is a resize event, which might invalidate some buffers
    // the renderer is using
    if (e.type == SDL_WINDOWEVENT and e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
        int w = e.window.data1;
        int h = e.window.data2;
        int samples = app.samples();
        impl->renderer.reallocate_buffers(w, h, samples);
        return true;
    }

    float aspect_ratio = app.window_aspect_ratio();
    auto window_dims = app.window_dimensions();

    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
        case SDLK_w:
            wireframe_mode = not wireframe_mode;
            return true;
        }
    } else if (e.type == SDL_MOUSEBUTTONDOWN) {
        switch (e.button.button) {
        case SDL_BUTTON_LEFT:
            dragging = true;
            return true;
        case SDL_BUTTON_RIGHT:
            panning = true;
            return true;
        }
    } else if (e.type == SDL_MOUSEBUTTONUP) {
        switch (e.button.button) {
        case SDL_BUTTON_LEFT:
            dragging = false;
            return true;
        case SDL_BUTTON_RIGHT:
            panning = false;
            return true;
        }
    } else if (e.type == SDL_MOUSEMOTION) {
        if (abs(e.motion.xrel) > 200 or abs(e.motion.yrel) > 200) {
            // probably a frameskip or the mouse was forcibly teleported
            // because it hit the edge of the screen
            return false;
        }

        if (dragging) {
            // alter camera position while dragging
            float dx = -static_cast<float>(e.motion.xrel) / static_cast<float>(window_dims.w);
            float dy = static_cast<float>(e.motion.yrel) / static_cast<float>(window_dims.h);
            theta += 2.0f * static_cast<float>(M_PI) * mouse_drag_sensitivity * dx;
            phi += 2.0f * static_cast<float>(M_PI) * mouse_drag_sensitivity * dy;
        }

        if (panning) {
            float dx = static_cast<float>(e.motion.xrel) / static_cast<float>(window_dims.w);
            float dy = -static_cast<float>(e.motion.yrel) / static_cast<float>(window_dims.h);

            // how much panning is done depends on how far the camera is from the
            // origin (easy, with polar coordinates) *and* the FoV of the camera.
            float x_amt = dx * aspect_ratio * (2.0f * tanf(fov / 2.0f) * radius);
            float y_amt = dy * (1.0f / aspect_ratio) * (2.0f * tanf(fov / 2.0f) * radius);

            // this assumes the scene is not rotated, so we need to rotate these
            // axes to match the scene's rotation
            glm::vec4 default_panning_axis = {x_amt, y_amt, 0.0f, 1.0f};
            auto rot_theta = glm::rotate(glm::identity<glm::mat4>(), theta, glm::vec3{0.0f, 1.0f, 0.0f});
            auto theta_vec = glm::normalize(glm::vec3{sinf(theta), 0.0f, cosf(theta)});
            auto phi_axis = glm::cross(theta_vec, glm::vec3{0.0, 1.0f, 0.0f});
            auto rot_phi = glm::rotate(glm::identity<glm::mat4>(), phi, phi_axis);

            glm::vec4 panning_axes = rot_phi * rot_theta * default_panning_axis;
            pan.x += panning_axes.x;
            pan.y += panning_axes.y;
            pan.z += panning_axes.z;
        }

        // wrap mouse if it hits edges
        if (dragging or panning) {
            constexpr int edge_width = 5;
            if (e.motion.x + edge_width > window_dims.w) {
                app.move_mouse_to(edge_width, e.motion.y);
            }
            if (e.motion.x - edge_width < 0) {
                app.move_mouse_to(window_dims.w - edge_width, e.motion.y);
            }
            if (e.motion.y + edge_width > window_dims.h) {
                app.move_mouse_to(e.motion.x, edge_width);
            }
            if (e.motion.y - edge_width < 0) {
                app.move_mouse_to(e.motion.x, window_dims.h - edge_width);
            }

            return true;
        }
    } else if (e.type == SDL_MOUSEWHEEL) {
        if (e.wheel.y > 0 and radius >= 0.1f) {
            radius *= mouse_wheel_sensitivity;
        }

        if (e.wheel.y <= 0 and radius < 100.0f) {
            radius /= mouse_wheel_sensitivity;
        }

        return true;
    }

    return false;
}

void osmv::Simple_model_renderer::draw(
    Application const& app, OpenSim::Model const& model, SimTK::State const& st, OpenSim::Component const* selected) {

    OpenSim_model_geometry& geom = impl->geom_swap;
    Raw_renderer& renderer = impl->renderer;

    // pull geometry out of the OpenSim model
    geom.clear();
    generate_geometry(model, st, geom);
    assert(geom.meshes.size() == geom.associated_components.size());

    // perform any necessary fixups on the geometry instances

    for (size_t i = 0; i < geom.meshes.size(); ++i) {
        Mesh_instance& mi = geom.meshes[i];

        // set passthrough data for hit-testing
        size_t id = i + 1;  // +1 because 0x0000 is reserved
        assert(id < (1 << 16) - 1);
        unsigned char b0 = id & 0xff;
        unsigned char b1 = (id >> 8) & 0xff;
        mi.set_passthrough_data(b0, b1);

        // if drawing selection rims, set the rims of selected/hovered components
        // accordingly
        if (draw_rims) {
            OpenSim::Component const* owner = geom.associated_components[i];
            if (selected != nullptr and selected == owner) {
                mi._passthrough.a = 1.0f;
            } else if (hovered_component != nullptr and hovered_component == owner) {
                mi._passthrough.a = 0.2f;
            } else {
                mi._passthrough.a = 0.0f;
            }
        }
    }

    // we can sort the mesh list now because we have encoded the index into `associated_components`
    // into each mesh instance
    renderer.sort_meshes_for_drawing(geom.meshes);

    // set hit-testing location based on mouse position
    //
    // - SDL screen coords are traditional screen coords. Origin top-left, Y goes down
    // - OpenGL screen coords are mathematical coords. Origin bottom-left, Y goes up
    sdl::Mouse_state m = sdl::GetMouseState();
    sdl::Window_dimensions d = app.window_dimensions();
    renderer.passthrough_hittest_x = m.x;
    renderer.passthrough_hittest_y = d.h - m.y;

    // set any other parameters that the raw renderer depends on
    renderer.view_matrix = compute_view_matrix(theta, phi, radius, pan);
    renderer.projection_matrix = glm::perspective(fov, app.window_aspect_ratio(), znear, zfar);
    renderer.view_pos = spherical_2_cartesian(theta, phi, radius);
    renderer.light_pos = light_pos;
    renderer.light_rgb = light_rgb;
    renderer.background_rgba = background_rgba;
    renderer.rim_rgba = rim_rgba;
    renderer.rim_thickness = rim_thickness;
    renderer.flags = RawRendererFlags_None;
    renderer.flags |= RawRendererFlags_PerformPassthroughHitTest;
    renderer.flags |= RawRendererFlags_UseOptimizedButDelayed1FrameHitTest;
    renderer.flags |= RawRendererFlags_DrawSceneGeometry;
    if (wireframe_mode) {
        renderer.flags |= RawRendererFlags_WireframeMode;
    }
    if (show_mesh_normals) {
        renderer.flags |= RawRendererFlags_ShowMeshNormals;
    }
    if (show_floor) {
        renderer.flags |= RawRendererFlags_ShowFloor;
    }
    if (draw_rims) {
        renderer.flags |= RawRendererFlags_DrawRims;
    }
    if (app.is_in_debug_mode()) {
        renderer.flags |= RawRendererFlags_DrawDebugQuads;
    }

    // perform draw call
    renderer.draw(geom.meshes);

    // post-draw: check if the hit-test passed
    // TODO:: optimized indices are from the previous frame, which might
    //        contain now-stale components
    unsigned char* bytes = renderer.passthrough_result_prev_frame;
    unsigned char b0 = bytes[0];
    unsigned char b1 = bytes[1];

    size_t id = static_cast<size_t>(b0);
    id |= static_cast<size_t>(b1) << 8;

    if (id == 0) {
        hovered_component = nullptr;
    } else {
        hovered_component = geom.associated_components[id - 1];
    }
}

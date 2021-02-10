#include "simple_model_renderer.hpp"

#include "raw_renderer.hpp"

#include "3d_common.hpp"
#include "application.hpp"
#include "sdl_wrapper.hpp"

#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/Muscle.h>
#include <SDL_events.h>
#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <SDL_mouse.h>
#include <SDL_video.h>
#include <SimTKcommon.h>
#include <SimTKcommon/Orientation.h>
#include <SimTKsimbody.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace OpenSim {
    class ModelDisplayHints;
}

namespace {
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
            vert_swap.clear();
            osmv::unit_sphere_triangles(vert_swap);
            sphere_meshid = osmv::globally_allocate_mesh(vert_swap.data(), vert_swap.size());

            vert_swap.clear();
            osmv::simbody_cylinder_triangles(vert_swap);
            cylinder_meshid = osmv::globally_allocate_mesh(vert_swap.data(), vert_swap.size());

            vert_swap.clear();
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
        void implementFrameGeometry(const DecorativeFrame& geom) override {
            glm::vec3 s = scale_factors(geom);
            s *= geom.getAxisLength();
            s *= 0.1f;

            glm::mat4 m = transform(geom);
            m = glm::scale(m, s);

            glm::vec4 rgba{1.0f, 0.0f, 0.0f, 1.0f};
            out.emplace_back(m, rgba, global_meshes().cylinder_meshid);
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
}

osmv::Simple_model_renderer::Simple_model_renderer(int w, int h, int samples) : renderer{w, h, samples} {
    OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();
}

osmv::Simple_model_renderer::~Simple_model_renderer() noexcept = default;

bool osmv::Simple_model_renderer::on_event(SDL_Event const& e) {
    Application& application = app();

    // edge-case: the event is a resize event, which might invalidate some buffers
    // the renderer is using
    if (e.type == SDL_WINDOWEVENT and e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
        int w = e.window.data1;
        int h = e.window.data2;
        int samples = application.samples();
        renderer.reallocate_buffers(w, h, samples);
        return true;
    }

    // other edge-case: the event is a sample count change
    if (e.type == SDL_USEREVENT and e.user.code == OsmvCustomEvent_SamplesChanged) {
        auto [w, h] = application.window_dimensions();
        int samples = application.samples();
        renderer.reallocate_buffers(w, h, samples);
        return true;
    }

    float aspect_ratio = app().window_aspect_ratio();
    auto window_dims = application.window_dimensions();

    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
        case SDLK_w:
            flags ^= SimpleModelRendererFlags_WireframeMode;
            return true;
        }
    } else if (e.type == SDL_MOUSEBUTTONDOWN) {
        switch (e.button.button) {
        case SDL_BUTTON_LEFT:
            flags |= SimpleModelRendererFlags_Dragging;
            return true;
        case SDL_BUTTON_RIGHT:
            flags |= SimpleModelRendererFlags_Panning;
            return true;
        }
    } else if (e.type == SDL_MOUSEBUTTONUP) {
        switch (e.button.button) {
        case SDL_BUTTON_LEFT:
            flags &= ~SimpleModelRendererFlags_Dragging;
            return true;
        case SDL_BUTTON_RIGHT:
            flags &= ~SimpleModelRendererFlags_Panning;
            return true;
        }
    } else if (e.type == SDL_MOUSEMOTION) {
        if (abs(e.motion.xrel) > 200 or abs(e.motion.yrel) > 200) {
            // probably a frameskip or the mouse was forcibly teleported
            // because it hit the edge of the screen
            return false;
        }

        if (flags & SimpleModelRendererFlags_Dragging) {
            // alter camera position while dragging
            float dx = -static_cast<float>(e.motion.xrel) / static_cast<float>(window_dims.w);
            float dy = static_cast<float>(e.motion.yrel) / static_cast<float>(window_dims.h);
            theta += 2.0f * static_cast<float>(M_PI) * mouse_drag_sensitivity * dx;
            phi += 2.0f * static_cast<float>(M_PI) * mouse_drag_sensitivity * dy;
        }

        if (flags & SimpleModelRendererFlags_Panning) {
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
        if (flags & (SimpleModelRendererFlags_Dragging | SimpleModelRendererFlags_Panning)) {
            constexpr int edge_width = 5;
            if (e.motion.x + edge_width > window_dims.w) {
                application.move_mouse_to(edge_width, e.motion.y);
            }
            if (e.motion.x - edge_width < 0) {
                application.move_mouse_to(window_dims.w - edge_width, e.motion.y);
            }
            if (e.motion.y + edge_width > window_dims.h) {
                application.move_mouse_to(e.motion.x, edge_width);
            }
            if (e.motion.y - edge_width < 0) {
                application.move_mouse_to(e.motion.x, window_dims.h - edge_width);
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

void osmv::Simple_model_renderer::generate_geometry(OpenSim::Model const& model, SimTK::State const& state) {
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

    geometry.clear();

    // get a reusable swap-space for geometry generation
    SimTK::Array_<SimTK::DecorativeGeometry>& dg_swap = global_meshes().dg_swap;

    // create a visitor that is called by OpenSim whenever it wants to generate abstract
    // geometry
    Geometry_visitor visitor{model.getSystem().getMatterSubsystem(), state, geometry.meshes};
    OpenSim::ModelDisplayHints const& hints = model.getDisplayHints();

    for (OpenSim::Component const& c : model.getComponentList()) {

        if (flags & osmv::SimpleModelRendererFlags_DrawStaticDecorations) {
            OpenSim::Component const* static_owner = nullptr;
            if (flags & osmv::SimpleModelRendererFlags_HoverableStaticDecorations) {
                static_owner = &c;
            }

            dg_swap.clear();
            c.generateDecorations(true, hints, state, dg_swap);

            size_t meshes_before = geometry.meshes.size();
            for (SimTK::DecorativeGeometry const& geom : dg_swap) {
                geom.implementGeometry(visitor);
            }
            size_t meshes_added = geometry.meshes.size() - meshes_before;

            for (size_t i = 0; i < meshes_added; ++i) {
                geometry.associated_components.push_back(static_owner);
            }

            assert(geometry.meshes.size() == geometry.associated_components.size());
        }

        if (flags & osmv::SimpleModelRendererFlags_DrawDynamicDecorations) {
            OpenSim::Component const* dynamic_owner = nullptr;
            if (flags & osmv::SimpleModelRendererFlags_HoverableDynamicDecorations) {
                dynamic_owner = &c;
            }

            dg_swap.clear();
            c.generateDecorations(false, hints, state, dg_swap);

            // populate append_out.meshes
            size_t meshes_before = geometry.meshes.size();
            for (SimTK::DecorativeGeometry const& geom : dg_swap) {
                geom.implementGeometry(visitor);
            }
            size_t meshes_added = geometry.meshes.size() - meshes_before;

            for (size_t i = 0; i < meshes_added; ++i) {
                geometry.associated_components.push_back(dynamic_owner);
            }

            assert(geometry.meshes.size() == geometry.associated_components.size());
        }
    }

    assert(geometry.meshes.size() == geometry.associated_components.size());
}

void osmv::Simple_model_renderer::apply_standard_rim_coloring(const OpenSim::Component* selected) {
    if (not(flags & SimpleModelRendererFlags_DrawRims)) {
        return;
    }

    if (selected == nullptr) {
        // replace with a senteniel because nullptr means "not assigned"
        // in the geometry list
        selected = reinterpret_cast<OpenSim::Component const*>(-1);
    }

    assert(geometry.meshes.size() == geometry.associated_components.size());
    for (size_t i = 0; i < geometry.meshes.size(); ++i) {
        Mesh_instance& mi = geometry.meshes[i];
        OpenSim::Component const* owner = geometry.associated_components[i];

        if (owner == selected) {
            mi._passthrough.a = 1.0f;
        } else if (hovered_component != nullptr and hovered_component == owner) {
            mi._passthrough.a = 0.2f;
        } else {
            mi._passthrough.a = 0.0f;
        }
    }
}

void osmv::Simple_model_renderer::draw() {
    // set passthrough data for hit-testing
    for (size_t i = 0; i < geometry.meshes.size(); ++i) {
        Mesh_instance& mi = geometry.meshes[i];

        size_t id = i + 1;  // +1 because 0x0000 is reserved
        assert(id < (1 << 16) - 1);
        unsigned char b0 = id & 0xff;
        unsigned char b1 = (id >> 8) & 0xff;
        mi.set_passthrough_data(b0, b1);
    }

    // we can sort the mesh list now because we have encoded the index into `associated_components`
    // into each mesh instance
    renderer.sort_meshes_for_drawing(geometry.meshes);

    // set hit-testing location based on mouse position
    //
    // - SDL screen coords are traditional screen coords. Origin top-left, Y goes down
    // - OpenGL screen coords are mathematical coords. Origin bottom-left, Y goes up
    sdl::Mouse_state m = sdl::GetMouseState();
    Window_dimensions d = app().window_dimensions();
    renderer.passthrough_hittest_x = m.x;
    renderer.passthrough_hittest_y = d.h - m.y;

    // set any other parameters that the raw renderer depends on
    renderer.view_matrix = compute_view_matrix(theta, phi, radius, pan);
    renderer.projection_matrix = glm::perspective(fov, app().window_aspect_ratio(), znear, zfar);
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
    if (flags & SimpleModelRendererFlags_WireframeMode) {
        renderer.flags |= RawRendererFlags_WireframeMode;
    }
    if (flags & SimpleModelRendererFlags_ShowMeshNormals) {
        renderer.flags |= RawRendererFlags_ShowMeshNormals;
    }
    if (flags & SimpleModelRendererFlags_ShowFloor) {
        renderer.flags |= RawRendererFlags_ShowFloor;
    }
    if (flags & SimpleModelRendererFlags_DrawRims) {
        renderer.flags |= RawRendererFlags_DrawRims;
    }
    if (app().is_in_debug_mode()) {
        renderer.flags |= RawRendererFlags_DrawDebugQuads;
    }

    // perform draw call
    renderer.draw(geometry.meshes);

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
        hovered_component = geometry.associated_components[id - 1];
    }
}

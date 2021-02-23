#include "model_drawlist_generator.hpp"

#include "meshes.hpp"
#include "raw_renderer.hpp"

#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/Muscle.h>
#include <SimTKcommon.h>
#include <SimTKcommon/Orientation.h>
#include <SimTKsimbody.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <unordered_map>
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
        osmv::Mesh_reference sphere_meshid;
        osmv::Mesh_reference cylinder_meshid;
        osmv::Mesh_reference cube_meshid;

        // path-to-meshid lookup
        //
        // allows decoration generators to lookup whether a mesh file (e.g. pelvis.vtp)
        // has already been uploaded to the GPU or not and, if it has, what meshid it
        // was assigned
        //
        // this is necessary because SimTK will emit mesh information as paths on the
        // filesystem
        std::unordered_map<std::string, osmv::Mesh_reference> path2meshid;

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
        osmv::Labelled_model_drawlist& out;
        std::function<void(osmv::ModelDrawlistOnAppendFlags, OpenSim::Component const*&, osmv::Raw_mesh_instance&)>&
            on_append;

        // caller mutates this as geometry is generated
        osmv::ModelDrawlistOnAppendFlags cur_flags = osmv::ModelDrawlistOnAppendFlags_None;
        OpenSim::Component const* cur_component = nullptr;

        Geometry_visitor(
            SimbodyMatterSubsystem const& _matter_subsystem,
            SimTK::State const& _state,
            osmv::Labelled_model_drawlist& _out,
            std::function<void(osmv::ModelDrawlistOnAppendFlags, OpenSim::Component const*&, osmv::Raw_mesh_instance&)>&
                _on_append) :

            matter_subsystem{_matter_subsystem},
            state{_state},
            out{_out},
            on_append{_on_append} {
        }

        template<typename... Args>
        void emplace_to_output(Args... args) {
            osmv::Model_drawlist_entry_reference ref = out.emplace_back(cur_component, std::forward<Args>(args)...);
            on_append(cur_flags, ref.component, ref.mesh_instance);
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

            // glm::mat4 is column major:
            //     see: https://glm.g-truc.net/0.9.2/api/a00001.html
            //     (and just Google "glm column major?")
            //
            // SimTK is whoknowswtf-major (actually, row), carefully read the
            // sourcecode for `SimTK::Transform`.

            glm::mat4 m;

            // x0 y0 z0 w0
            Rotation const& r = t.R();
            Vec3 const& p = t.p();

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

            m[0][3] = 0.0f;
            m[1][3] = 0.0f;
            m[2][3] = 0.0f;
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

        osmv::Rgba32 rgba(DecorativeGeometry const& geom) {
            Vec3 const& rgb = geom.getColor();
            Real a = geom.getOpacity();

            osmv::Rgba32 rv;
            rv.r = static_cast<unsigned char>(255.0 * rgb[0]);
            rv.g = static_cast<unsigned char>(255.0 * rgb[1]);
            rv.b = static_cast<unsigned char>(255.0 * rgb[2]);
            rv.a = a < 0.0 ? 255 : static_cast<unsigned char>(255.0 * a);
            return rv;
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

            emplace_to_output(cylinder_xform, rgba(geom), global_meshes().cylinder_meshid);
        }
        void implementBrickGeometry(const DecorativeBrick& geom) override {
            SimTK::Vec3 dims = geom.getHalfLengths();
            glm::mat4 xform = glm::scale(transform(geom), glm::vec3{dims[0], dims[1], dims[2]});

            emplace_to_output(xform, rgba(geom), global_meshes().cube_meshid);
        }
        void implementCylinderGeometry(const DecorativeCylinder& geom) override {
            glm::mat4 m = transform(geom);
            glm::vec3 s = scale_factors(geom);
            s.x *= static_cast<float>(geom.getRadius());
            s.y *= static_cast<float>(geom.getHalfHeight());
            s.z *= static_cast<float>(geom.getRadius());

            glm::mat4 xform = glm::scale(m, s);

            emplace_to_output(xform, rgba(geom), global_meshes().cylinder_meshid);
        }
        void implementCircleGeometry(const DecorativeCircle&) override {
            // nyi
        }
        void implementSphereGeometry(const DecorativeSphere& geom) override {
            float r = static_cast<float>(geom.getRadius());
            glm::mat4 xform = glm::scale(transform(geom), glm::vec3{r, r, r});

            emplace_to_output(xform, rgba(geom), global_meshes().sphere_meshid);
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

            emplace_to_output(m, rgba, global_meshes().cylinder_meshid);
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
            osmv::Mesh_reference meshid = osmv::Mesh_reference::invalid();
            {
                auto [it, inserted] = global.path2meshid.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(std::ref(m.getMeshFile())),
                    std::forward_as_tuple(osmv::Mesh_reference::invalid()));

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
            emplace_to_output(xform, rgba(m), meshid);
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
}

namespace osmv {
    struct Drawlist_generator_impl final {
        SimTK::Array_<SimTK::DecorativeGeometry> dg_swap;
    };
}

osmv::Model_drawlist_generator::Model_drawlist_generator() : impl{new Drawlist_generator_impl{}} {
}

osmv::Model_drawlist_generator::~Model_drawlist_generator() noexcept {
    delete impl;
}

void osmv::Model_drawlist_generator::generate(
    OpenSim::Model const& model,
    SimTK::State const& state,
    Labelled_model_drawlist& out,
    std::function<void(ModelDrawlistOnAppendFlags, OpenSim::Component const*&, Raw_mesh_instance&)> on_append,
    ModelDrawlistGeneratorFlags flags) {

    // get a reusable swap-space for geometry generation
    SimTK::Array_<SimTK::DecorativeGeometry>& dg_swap = impl->dg_swap;

    // create a visitor that is called by OpenSim whenever it wants to generate abstract
    // geometry
    Geometry_visitor visitor{model.getSystem().getMatterSubsystem(), state, out, on_append};
    OpenSim::ModelDisplayHints const& hints = model.getDisplayHints();

    for (OpenSim::Component const& c : model.getComponentList()) {
        if (flags & ModelDrawlistGeneratorFlags_GenerateStaticDecorations) {
            dg_swap.clear();
            c.generateDecorations(true, hints, state, dg_swap);

            visitor.cur_component = &c;
            visitor.cur_flags = ModelDrawlistOnAppendFlags_IsStatic;
            for (SimTK::DecorativeGeometry const& geom : dg_swap) {
                geom.implementGeometry(visitor);
            }
        }

        if (flags & ModelDrawlistGeneratorFlags_GenerateDynamicDecorations) {
            dg_swap.clear();
            c.generateDecorations(false, hints, state, dg_swap);

            visitor.cur_component = &c;
            visitor.cur_flags = ModelDrawlistOnAppendFlags_IsDynamic;
            for (SimTK::DecorativeGeometry const& geom : dg_swap) {
                geom.implementGeometry(visitor);
            }
        }
    }
}

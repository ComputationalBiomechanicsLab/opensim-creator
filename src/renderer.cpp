#include "renderer.hpp"

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
    // renders to two render targets:
    //
    // - COLOR0: uniform-colored geometry with Gouraud shading
    // - COLOR1: whatever uRgba2 is set to, with no modification
    struct Gouraud_mrt_shader final {
        gl::Program program = gl::CreateProgramFrom(
            gl::Compile<gl::Vertex_shader>(osmv::cfg::shader_path("gouraud_mrt.vert")),
            gl::Compile<gl::Fragment_shader>(osmv::cfg::shader_path("gouraud_mrt.frag")));

        static constexpr gl::Attribute aLocation = gl::AttributeAtLocation(0);
        static constexpr gl::Attribute aNormal = gl::AttributeAtLocation(1);

        gl::Uniform_mat4 uProjMat = gl::GetUniformLocation(program, "uProjMat");
        gl::Uniform_mat4 uViewMat = gl::GetUniformLocation(program, "uViewMat");
        gl::Uniform_mat4 uModelMat = gl::GetUniformLocation(program, "uModelMat");
        gl::Uniform_mat4 uNormalMat = gl::GetUniformLocation(program, "uNormalMat");
        gl::Uniform_vec4 uRgba = gl::GetUniformLocation(program, "uRgba");
        gl::Uniform_vec3 uLightPos = gl::GetUniformLocation(program, "uLightPos");
        gl::Uniform_vec3 uLightColor = gl::GetUniformLocation(program, "uLightColor");
        gl::Uniform_vec3 uViewPos = gl::GetUniformLocation(program, "uViewPos");
        gl::Uniform_vec4 uRgba2 = gl::GetUniformLocation(program, "uRgba2");

        template<typename Vbo, typename T = typename Vbo::value_type>
        static gl::Vertex_array create_vao(Vbo& vbo) {
            gl::Vertex_array vao = gl::GenVertexArrays();

            gl::BindVertexArray(vao);
            gl::BindBuffer(vbo);
            gl::VertexAttribPointer(
                aLocation, 3, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, pos)));
            gl::EnableVertexAttribArray(aLocation);
            gl::VertexAttribPointer(
                aNormal, 3, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, normal)));
            gl::EnableVertexAttribArray(aNormal);
            gl::BindVertexArray();

            return vao;
        }
    };

    // renders textured geometry with no shading at all
    struct Plain_texture_shader final {
        gl::Program p = gl::CreateProgramFrom(
            gl::Compile<gl::Vertex_shader>(osmv::cfg::shader_path("plain_texture.vert")),
            gl::Compile<gl::Fragment_shader>(osmv::cfg::shader_path("plain_texture.frag")));

        static constexpr gl::Attribute aPos{0};
        static constexpr gl::Attribute aTexCoord{1};

        gl::Uniform_mat4 projMat = {p, "projMat"};
        gl::Uniform_mat4 viewMat = {p, "viewMat"};
        gl::Uniform_mat4 modelMat = {p, "modelMat"};
        gl::Uniform_sampler2d uSampler0 = {p, "uSampler0"};

        template<typename Vbo, typename T = typename Vbo::value_type>
        static gl::Vertex_array create_vao(Vbo& vbo) {
            gl::Vertex_array vao = gl::GenVertexArrays();

            gl::BindVertexArray(vao);
            gl::BindBuffer(vbo);
            gl::VertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, pos)));
            gl::EnableVertexAttribArray(aPos);
            gl::VertexAttribPointer(
                aTexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, texcoord)));
            gl::EnableVertexAttribArray(aTexCoord);
            gl::BindVertexArray();

            return vao;
        }
    };

    // renders uniform-colored geometry with no shading at all
    struct Plain_color_shader final {
        gl::Program p = gl::CreateProgramFrom(
            gl::Compile<gl::Vertex_shader>(osmv::cfg::shader_path("plain_color.vert")),
            gl::Compile<gl::Fragment_shader>(osmv::cfg::shader_path("plain_color.frag")));

        static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);

        gl::Uniform_mat4 uModelMat = gl::GetUniformLocation(p, "uModelMat");
        gl::Uniform_mat4 uViewMat = gl::GetUniformLocation(p, "uViewMat");
        gl::Uniform_mat4 uProjMat = gl::GetUniformLocation(p, "uProjMat");
        gl::Uniform_vec3 uRgb = gl::GetUniformLocation(p, "uRgb");

        template<typename Vbo, typename T = typename Vbo::value_type>
        static gl::Vertex_array create_vao(Vbo& vbo) {
            gl::Vertex_array vao = gl::GenVertexArrays();

            gl::BindVertexArray(vao);
            gl::BindBuffer(vbo);
            gl::VertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, pos)));
            gl::EnableVertexAttribArray(aPos);
            gl::BindVertexArray();

            return vao;
        }
    };

    struct Edge_detection_shader final {
        gl::Program p = gl::CreateProgramFrom(
            gl::Compile<gl::Vertex_shader>(osmv::cfg::shader_path("edge_detect.vert")),
            gl::Compile<gl::Fragment_shader>(osmv::cfg::shader_path("edge_detect.frag")));

        static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
        static constexpr gl::Attribute aTexCoord = gl::AttributeAtLocation(1);

        gl::Uniform_mat4 uModelMat = gl::GetUniformLocation(p, "uModelMat");
        gl::Uniform_mat4 uViewMat = gl::GetUniformLocation(p, "uViewMat");
        gl::Uniform_mat4 uProjMat = gl::GetUniformLocation(p, "uProjMat");
        gl::Uniform_vec4 uBackgroundColor = gl::GetUniformLocation(p, "uBackgroundColor");
        gl::Uniform_sampler2d uSamplerSceneColors = gl::GetUniformLocation(p, "uSamplerSceneColors");
        gl::Uniform_sampler2d uSamplerSelectionEdges = gl::GetUniformLocation(p, "uSamplerSelectionEdges");

        template<typename Vbo, typename T = typename Vbo::value_type>
        static gl::Vertex_array create_vao(Vbo& vbo) {
            gl::Vertex_array vao = gl::GenVertexArrays();

            gl::BindVertexArray(vao);
            gl::BindBuffer(vbo);
            gl::VertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, pos)));
            gl::EnableVertexAttribArray(aPos);
            gl::VertexAttribPointer(
                aTexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, texcoord)));
            gl::EnableVertexAttribArray(aTexCoord);
            gl::BindVertexArray();

            return vao;
        }
    };

    struct Skip_msxaa_blitter_shader final {
        gl::Program p = gl::CreateProgramFrom(
            gl::Compile<gl::Vertex_shader>(osmv::cfg::shader_path("skip_msxaa_blitter.vert")),
            gl::Compile<gl::Fragment_shader>(osmv::cfg::shader_path("skip_msxaa_blitter.frag")));

        static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
        static constexpr gl::Attribute aTexCoord = gl::AttributeAtLocation(1);

        gl::Uniform_mat4 uModelMat = gl::GetUniformLocation(p, "uModelMat");
        gl::Uniform_mat4 uViewMat = gl::GetUniformLocation(p, "uViewMat");
        gl::Uniform_mat4 uProjMat = gl::GetUniformLocation(p, "uProjMat");
        gl::Uniform_sampler2DMS uSampler0 = gl::GetUniformLocation(p, "uSampler0");

        template<typename Vbo, typename T = typename Vbo::value_type>
        static gl::Vertex_array create_vao(Vbo& vbo) {
            gl::Vertex_array vao = gl::GenVertexArrays();

            gl::BindVertexArray(vao);
            gl::BindBuffer(vbo);
            gl::VertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, pos)));
            gl::EnableVertexAttribArray(aPos);
            gl::VertexAttribPointer(
                aTexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, texcoord)));
            gl::EnableVertexAttribArray(aTexCoord);
            gl::BindVertexArray();

            return vao;
        }
    };

    // uses a geometry shader to render normals as lines
    struct Normals_shader final {
        gl::Program program = gl::CreateProgramFrom(
            gl::Compile<gl::Vertex_shader>(osmv::cfg::shader_path("draw_normals.vert")),
            gl::Compile<gl::Fragment_shader>(osmv::cfg::shader_path("draw_normals.frag")),
            gl::Compile<gl::Geometry_shader>(osmv::cfg::shader_path("draw_normals.geom")));

        static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
        static constexpr gl::Attribute aNormal = gl::AttributeAtLocation(1);

        gl::Uniform_mat4 uModelMat = gl::GetUniformLocation(program, "uModelMat");
        gl::Uniform_mat4 uViewMat = gl::GetUniformLocation(program, "uViewMat");
        gl::Uniform_mat4 uProjMat = gl::GetUniformLocation(program, "uProjMat");
        gl::Uniform_mat4 uNormalMat = gl::GetUniformLocation(program, "uNormalMat");

        template<typename Vbo, typename T = typename Vbo::value_type>
        static gl::Vertex_array create_vao(Vbo& vbo) {
            gl::Vertex_array vao = gl::GenVertexArrays();
            gl::BindVertexArray(vao);
            gl::BindBuffer(vbo);
            gl::VertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, pos)));
            gl::EnableVertexAttribArray(aPos);
            gl::VertexAttribPointer(
                aNormal, 3, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, normal)));
            gl::EnableVertexAttribArray(aNormal);
            gl::BindVertexArray();
            return vao;
        }
    };

    // mesh, fully loaded onto the GPU with whichever VAOs it needs initialized also
    struct Mesh_on_gpu final {
        gl::Array_bufferT<osmv::Untextured_vert> vbo;
        gl::Vertex_array main_vao;
        gl::Vertex_array normal_vao;
        gl::Vertex_array plain_color_vao;

    public:
        Mesh_on_gpu(std::vector<osmv::Untextured_vert> const& m) :
            vbo{m},
            main_vao{Gouraud_mrt_shader::create_vao(vbo)},
            normal_vao{Normals_shader::create_vao(vbo)},
            plain_color_vao{Plain_color_shader::create_vao(vbo)} {
        }

        int sizei() const noexcept {
            return vbo.sizei();
        }
    };

    // global mesh loader state
    //
    // mesh IDs are guaranteed to be globally unique application-wide and monotonically
    // increase from 1, mesh loading is designed to minimize runtime allocations.
    //
    // although globals are evil, there's good reasons for doing this in osmv:
    //
    // - It means that draw calls for different OpenSim models/states, different geometries,
    //   etc. can share exactly the same GPU-stored mesh data, which means that loading+rendering
    //   a bunch of different OpenSim model files can be fast
    //
    // - Global uniqueness also means that instanced rendering is possible. All instances with
    //   the same meshid *definitely* have the same vertices GPU-side, so draw calls can be
    //   performed in an instanced fashion, rather than step-by-step (fast)
    //
    // - Monotonically increasing means that mesh lookups can use a global LUT that's contiguous
    //   in memory without having to use (e.g.) a hashtable. Effectively, looking up a GPU-side
    //   mesh costs however much it costs to lookup an item in an array (fast)
    //
    // - We're using OpenGL for GPU interaction, so it's unlikely we need a threadsafe renderer
    //
    // Disadvantages:
    //
    // - No way to deallocate the mesh once it's on the GPU. However, OpenSim meshes are very
    //   simple and low-memory (e.g. they're not heavy game assets /w many textures spanning
    //   multiple levels)
    //
    // - Need to be careful with initialization (must happen *after* OpenGL is initialized).
    //   That's handled with a function-local static that's called when the global LUT is
    //   first needed (after OpenGL initialization)
    //
    // - Need to be careful with multithreading. This isn't handled, because this LUT relies on
    //   OpenGL, which *definitely* requires a well-understood thread model.
    //
    // most of the b.s. in this datastructure exists because OpenSim/Simbody aggressively
    // (badly) preload mesh data and attach it to model instances etc. This is exactly the
    // opposite of what's necessary to implement a high-perf content-addressible geometry
    // freewheel, but *shrug*.
    struct Global_mesh_loader_state final {

        // this is *never* a valid meshid, so if any datastructure contains one, then
        // something has screwed up somewhere (e.g. the implementation threw when using
        // this senteniel for an intermediate state)
        static constexpr int invalid_meshid = -1;

        // reserved mesh IDs:
        //
        // these are meshes that aren't actually loaded from a file, but generated. Things like
        // spheres and planes fall into this category. They are typically generated on the CPU
        // once and then uploaded onto the GPU. Then, whenever OpenSim/Simbody want one they can
        // just use the meshid to automatically freewheel it from the GPU.
        static constexpr int sphere_meshid = 0;
        static constexpr int cylinder_meshid = 1;
        static constexpr int cube_meshid = 2;

        // Handles for alread-uploaded meshes, indexed by meshid
        //
        // all optimal runtime paths should try to use this. It's a straight lookup
        // into a GPU-side mesh
        std::vector<Mesh_on_gpu> meshes;

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

        Global_mesh_loader_state() {
            // allocate reserved meshes
            static_assert(sphere_meshid == 0);
            osmv::unit_sphere_triangles(vert_swap);
            meshes.emplace_back(vert_swap);

            static_assert(cylinder_meshid == 1);
            osmv::simbody_cylinder_triangles(vert_swap);
            meshes.emplace_back(vert_swap);

            static_assert(cube_meshid == 2);
            osmv::simbody_brick_triangles(vert_swap);
            meshes.emplace_back(vert_swap);
        }
    };

    // getter for the global mesh loader instance
    //
    // must only be called after OpenGL is initialized.
    Global_mesh_loader_state& global_meshes() {
        static Global_mesh_loader_state st;
        return st;
    }

    Mesh_on_gpu& global_mesh_lookup(int meshid) {
        std::vector<Mesh_on_gpu>& meshes = global_meshes().meshes;

        assert(meshid != Global_mesh_loader_state::invalid_meshid);
        assert(meshid >= 0);
        assert(static_cast<size_t>(meshid) < meshes.size());

        return meshes[static_cast<size_t>(meshid)];
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
        OpenSim::Model const& model;
        SimTK::State const& state;
        std::vector<osmv::Mesh_instance>& out;

        // set by `set_current_component`, used by other steps of the process to "label" each
        // piece of geometry
        Component const* current_component = nullptr;

        Geometry_visitor(
            OpenSim::Model const& _model, SimTK::State const& _state, std::vector<osmv::Mesh_instance>& _out) :
            model{_model},
            state{_state},
            out{_out} {
        }

        void set_current_component(Component const* component) {
            current_component = component;
        }

        Transform ground_to_decoration_xform(DecorativeGeometry const& geom) {
            SimbodyMatterSubsystem const& ms = model.getSystem().getMatterSubsystem();
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

            out.emplace_back(current_component, cylinder_xform, rgba(geom), Global_mesh_loader_state::cylinder_meshid);
        }
        void implementBrickGeometry(const DecorativeBrick& geom) override {
            SimTK::Vec3 dims = geom.getHalfLengths();
            glm::mat4 xform = glm::scale(transform(geom), glm::vec3{dims[0], dims[1], dims[2]});

            out.emplace_back(current_component, xform, rgba(geom), Global_mesh_loader_state::cube_meshid);
        }
        void implementCylinderGeometry(const DecorativeCylinder& geom) override {
            glm::mat4 m = transform(geom);
            glm::vec3 s = scale_factors(geom);
            s.x *= static_cast<float>(geom.getRadius());
            s.y *= static_cast<float>(geom.getHalfHeight());
            s.z *= static_cast<float>(geom.getRadius());

            glm::mat4 xform = glm::scale(m, s);

            out.emplace_back(current_component, xform, rgba(geom), Global_mesh_loader_state::cylinder_meshid);
        }
        void implementCircleGeometry(const DecorativeCircle&) override {
            // nyi
        }
        void implementSphereGeometry(const DecorativeSphere& geom) override {
            float r = static_cast<float>(geom.getRadius());
            glm::mat4 xform = glm::scale(transform(geom), glm::vec3{r, r, r});

            out.emplace_back(current_component, xform, rgba(geom), Global_mesh_loader_state::sphere_meshid);
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
            Global_mesh_loader_state& gmls = global_meshes();

            // perform a cache search for the mesh
            int meshid = Global_mesh_loader_state::invalid_meshid;
            {
                auto [it, inserted] = gmls.path2meshid.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(std::ref(m.getMeshFile())),
                    std::forward_as_tuple(Global_mesh_loader_state::invalid_meshid));

                if (not inserted) {
                    // it wasn't inserted, so the path has already been loaded and the entry
                    // contains a meshid for the fully-loaded mesh
                    meshid = it->second;
                } else {
                    // it was inserted, and is currently a junk meshid because we haven't loaded
                    // a mesh yet. Load the mesh from the file onto the GPU and allocate a new
                    // meshid for it. Assign that meshid to the path2meshid lookup.
                    load_mesh_data(m.getMesh(), gmls.vert_swap);

                    // ensure there's enough space in the meshid space for an extra entry
                    assert(gmls.meshes.size() < std::numeric_limits<int>::max());

                    meshid = static_cast<int>(gmls.meshes.size());
                    it->second = meshid;
                    gmls.meshes.emplace_back(gmls.vert_swap);
                }
            }

            glm::mat4 xform = glm::scale(transform(m), scale_factors(m));
            out.emplace_back(current_component, xform, rgba(m), meshid);
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

    // create an OpenGL Pixel Buffer Object (PBO) that holds exactly one pixel
    gl::Pixel_pack_buffer make_single_pixel_PBO() {
        gl::Pixel_pack_buffer rv;
        gl::BindBuffer(rv);
        GLubyte rgba[4]{};  // initialize PBO's content to zeroed values
        gl::BufferData(rv.type, 4, rgba, GL_STREAM_READ);
        gl::UnbindBuffer(rv);
        return rv;
    }

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

namespace osmv {
    struct Restore_original_framebuffer_on_destruction final {
        GLint original_draw_fbo;
        GLint original_read_fbo;

        Restore_original_framebuffer_on_destruction() {
            glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &original_draw_fbo);
            glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &original_read_fbo);
        }
        Restore_original_framebuffer_on_destruction(Restore_original_framebuffer_on_destruction const&) = delete;
        Restore_original_framebuffer_on_destruction(Restore_original_framebuffer_on_destruction&&) = delete;
        Restore_original_framebuffer_on_destruction&
            operator=(Restore_original_framebuffer_on_destruction const&) = delete;
        Restore_original_framebuffer_on_destruction& operator=(Restore_original_framebuffer_on_destruction&&) = delete;
        ~Restore_original_framebuffer_on_destruction() noexcept {
            gl::BindFrameBuffer(GL_DRAW_FRAMEBUFFER, original_draw_fbo);
            gl::BindFrameBuffer(GL_READ_FRAMEBUFFER, original_read_fbo);
        }
    };

    // OpenGL buffers used by the renderer
    //
    // designed with move + assignment semantics in-mind, so that users can just
    // reassign new Render_buffers over these ones (e.g. if window dimensions
    // change)
    struct Renderer_buffers {

        // dimensions that these buffers were initialized with
        sdl::Window_dimensions dims;

        // num multisamples these buffers were initialized with
        int samples;

        // stores multisampled OpenSim scene
        gl::Texture_2d_multisample gColor0_mstex;

        // stores multisampled item index and selection rim alphas
        gl::Texture_2d_multisample gColor1_mstex;

        // stores multisampled depth + stencil values for the main MRT framebuffer
        gl::Render_buffer gDepth24Stencil8_rbo;

        // main MRT framebuffer
        gl::Frame_buffer gMRT_fbo;

        // stores non-MSXAAed version of the index and selection data (COLOR1)
        gl::Texture_2d gSkipMSXAA_tex;

        // framebuffer for non-MSXAAed index+selection render
        gl::Frame_buffer gSkipMSXAA_fbo;

        // stores resolved (post-MSXAA) OpenSim scene
        gl::Texture_2d gColor0_resolved;

        // fbo for resolved (post-MSXAA) OpenSim scene
        gl::Frame_buffer gColor0_resolved_fbo;

        // stores resolved (post-MSXAA) index + selection rim alphas
        gl::Texture_2d gColor1_resolved;

        // fbo for resolving color1 via a framebuffer blit
        gl::Frame_buffer gColor1_resolved_fbo;

        // pixel buffer objects (PBOs) for storing pixel color values
        //
        // these are used to asychronously request the pixel under the user's mouse
        // such that the renderer can decode that pixel value *on the next frame*
        // without stalling the GPU pipeline
        std::array<gl::Pixel_pack_buffer, 2> pbos{make_single_pixel_PBO(), make_single_pixel_PBO()};
        int pbo_idx = 0;  // 0 or 1

        // TODO: the renderer may not necessarily be drawing into the application screen
        //       and may, instead, be drawing into an arbitrary FBO (e.g. for a panel, or
        //       video recording), so the renderer shouldn't assume much about the app
        Renderer_buffers(sdl::Window_dimensions _dims, int _samples) :
            dims{_dims},
            samples{_samples},

            // allocate COLOR0: multisampled RGBA texture for scene
            gColor0_mstex{[this]() {
                gl::Texture_2d_multisample rv;
                gl::BindTexture(rv);
                glTexImage2DMultisample(rv.type, samples, GL_RGBA, dims.w, dims.h, GL_TRUE);
                return rv;
            }()},

            // allocate COLOR1: multisampled RGBA texture for selection logic
            gColor1_mstex{[this]() {
                gl::Texture_2d_multisample rv;
                gl::BindTexture(rv);
                glTexImage2DMultisample(rv.type, samples, GL_RGBA, dims.w, dims.h, GL_TRUE);
                return rv;
            }()},

            // allocate DEPTH+STENCIL: multisampled RBOs needed to "complete" the MRT FBO
            gDepth24Stencil8_rbo{[this]() {
                gl::Render_buffer rv;
                gl::BindRenderBuffer(rv);
                glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, dims.w, dims.h);
                return rv;
            }()},

            // allocate MRT FBO that scene draws into
            gMRT_fbo{[this]() {
                Restore_original_framebuffer_on_destruction restore_fbos;

                gl::Frame_buffer rv;

                // configure main FBO
                gl::BindFrameBuffer(GL_FRAMEBUFFER, rv);
                gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gColor0_mstex.type, gColor0_mstex, 0);
                gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, gColor1_mstex.type, gColor1_mstex, 0);
                gl::FramebufferRenderbuffer(
                    GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gDepth24Stencil8_rbo);

                // main FBO allocated, check it's OK
                assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

                return rv;
            }()},

            // allocate non-MSXAAed texture for non-blended hover detection
            gSkipMSXAA_tex{[this]() {
                gl::Texture_2d rv;

                // allocate non-MSXAA texture for non-blended sampling
                gl::BindTexture(rv);
                gl::TexImage2D(rv.type, 0, GL_RGBA, dims.w, dims.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

                return rv;
            }()},

            // allocate non-MSXAAed FBO for the non-blended write
            gSkipMSXAA_fbo{[this]() {
                Restore_original_framebuffer_on_destruction restore_fbos;

                gl::Frame_buffer rv;

                // configure non-MSXAA fbo
                gl::BindFrameBuffer(GL_FRAMEBUFFER, rv);
                gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gSkipMSXAA_tex.type, gSkipMSXAA_tex, 0);

                // check non-MSXAA OK
                assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

                return rv;
            }()},

            // allocate resolved (post-MSXAA) COLOR0 (OpenSim scene) texture
            gColor0_resolved{[this]() {
                gl::Texture_2d rv;
                gl::BindTexture(rv);
                gl::TexImage2D(rv.type, 0, GL_RGBA, dims.w, dims.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
                gl::TextureParameteri(rv, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // no mipmaps
                gl::TextureParameteri(rv, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // no mipmaps
                return rv;
            }()},

            // allocate FBO for resolved (post-MSXAA) COLOR0 (OpenSim scene) texture
            gColor0_resolved_fbo{[this]() {
                Restore_original_framebuffer_on_destruction restore_fbos;

                gl::Frame_buffer rv;
                gl::BindFrameBuffer(GL_FRAMEBUFFER, rv);
                gl::FramebufferTexture2D(
                    GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gColor0_resolved.type, gColor0_resolved, 0);

                assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

                return rv;
            }()},

            // allocate resolved (post-MSXAA) COLOR1 (selection logic) texture
            gColor1_resolved{[this]() {
                gl::Texture_2d rv;
                gl::BindTexture(rv);
                gl::TexImage2D(rv.type, 0, GL_RGBA, dims.w, dims.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
                gl::TextureParameteri(rv, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // no mipmaps
                gl::TextureParameteri(rv, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // no mipmaps
                return rv;
            }()},

            gColor1_resolved_fbo{[this]() {
                Restore_original_framebuffer_on_destruction restore_fbos;

                gl::Frame_buffer rv;
                gl::BindFrameBuffer(GL_FRAMEBUFFER, rv);
                gl::FramebufferTexture2D(
                    GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gColor1_resolved.type, gColor1_resolved, 0);

                assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

                return rv;
            }()} {
        }
    };

    // internal renderer implementation details
    struct Renderer_impl final {

        struct {
            Gouraud_mrt_shader gouraud;
            Normals_shader normals;
            Plain_color_shader plain_color;
            Plain_texture_shader plain_texture;
            Edge_detection_shader edge_detection_shader;
            Skip_msxaa_blitter_shader skip_msxaa_shader;
        } shaders;

        // debug quad
        gl::Array_bufferT<osmv::Shaded_textured_vert> quad_vbo = osmv::shaded_textured_quad_verts;
        gl::Vertex_array edge_detection_quad_vao = Edge_detection_shader::create_vao(quad_vbo);
        gl::Vertex_array skip_msxaa_quad_vao = Skip_msxaa_blitter_shader::create_vao(quad_vbo);
        gl::Vertex_array pts_quad_vao = Plain_texture_shader::create_vao(quad_vbo);

        // floor texture
        struct {
            gl::Array_bufferT<osmv::Shaded_textured_vert> vbo = []() {
                auto copy = osmv::shaded_textured_quad_verts;
                for (osmv::Shaded_textured_vert& st : copy) {
                    st.texcoord *= 25.0f;  // make chequers smaller
                }
                return gl::Array_bufferT<osmv::Shaded_textured_vert>{copy};
            }();

            gl::Vertex_array vao = Plain_texture_shader::create_vao(vbo);
            gl::Texture_2d floor_texture = osmv::generate_chequered_floor_texture();
            glm::mat4 model_mtx = []() {
                glm::mat4 rv = glm::identity<glm::mat4>();
                rv = glm::rotate(rv, osmv::pi_f / 2, {-1.0, 0.0, 0.0});
                rv = glm::scale(rv, {100.0f, 100.0f, 0.0f});
                return rv;
            }();
        } floor;

        // other OpenGL (GPU) buffers used by the renderer
        Renderer_buffers buffers;

        Renderer_impl(Application const& app) : buffers{app.window_dimensions(), app.samples()} {
        }
    };
}

// ok, this took an inordinate amount of time, but there's a fucking
// annoying bug in Clang:
//
// https://bugs.llvm.org/show_bug.cgi?id=28280
//
// where you'll get insane link errors if you use curly bracers to init
// the state. I *think* it's because there's a mix of noexcept and except
// dtors (Simbody uses exceptional dtors...)
//
// DO NOT USE CURLY BRACERS HERE
osmv::Renderer::Renderer(osmv::Application const& app) : state(new Renderer_impl(app)) {
}

osmv::Renderer::~Renderer() noexcept = default;

osmv::Event_response osmv::Renderer::on_event(Application& app, SDL_Event const& e) {
    // edge-case: the event is a resize event, which might invalidate some buffers
    // the renderer is using
    if (e.type == SDL_WINDOWEVENT and e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
        sdl::Window_dimensions new_dims{e.window.data1, e.window.data2};

        if (state->buffers.dims != new_dims) {
            // don't try and do anything fancy like reallocate or resize the existing
            // buffers, just allocate new ones and assign over
            state->buffers = Renderer_buffers{app.window_dimensions(), app.samples()};
        }

        return Event_response::handled;
    }

    float aspect_ratio = app.window_aspect_ratio();
    auto window_dims = app.window_dimensions();

    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
        case SDLK_w:
            wireframe_mode = not wireframe_mode;
            return Event_response::handled;
        }
    } else if (e.type == SDL_MOUSEBUTTONDOWN) {
        switch (e.button.button) {
        case SDL_BUTTON_LEFT:
            dragging = true;
            return Event_response::handled;
        case SDL_BUTTON_RIGHT:
            panning = true;
            return Event_response::handled;
        }
    } else if (e.type == SDL_MOUSEBUTTONUP) {
        switch (e.button.button) {
        case SDL_BUTTON_LEFT:
            dragging = false;
            return Event_response::handled;
        case SDL_BUTTON_RIGHT:
            panning = false;
            return Event_response::handled;
        }
    } else if (e.type == SDL_MOUSEMOTION) {
        if (abs(e.motion.xrel) > 200 or abs(e.motion.yrel) > 200) {
            // probably a frameskip or the mouse was forcibly teleported
            // because it hit the edge of the screen
            return Event_response::ignored;
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

            return Event_response::handled;
        }
    } else if (e.type == SDL_MOUSEWHEEL) {
        if (e.wheel.y > 0 and radius >= 0.1f) {
            radius *= mouse_wheel_sensitivity;
        }

        if (e.wheel.y <= 0 and radius < 100.0f) {
            radius /= mouse_wheel_sensitivity;
        }

        return Event_response::handled;
    }

    return Event_response::ignored;
}

void osmv::Renderer::generate_geometry(
    OpenSim::Model const& model, SimTK::State const& st, State_geometry& out, GeometryGeneratorFlags flags) {

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

    out.clear();

    SimTK::Array_<SimTK::DecorativeGeometry>& dg_swap = global_meshes().dg_swap;
    Geometry_visitor visitor{model, st, out.meshes};
    OpenSim::ModelDisplayHints const& hints = model.getDisplayHints();

    for (OpenSim::Component const& c : model.getComponentList()) {
        // HACK: traverse up the component tree until a muscle or the root is hit
        Component const* owner = nullptr;
        for (Component const* p = &c; p != &model; p = &p->getOwner()) {
            if (dynamic_cast<OpenSim::Muscle const*>(p)) {
                owner = p;
                break;
            }
        }

        dg_swap.clear();

        if (flags & GeometryGeneratorFlags_Static) {
            visitor.set_current_component(nullptr);  // static geometry has no owner
            c.generateDecorations(true, hints, st, dg_swap);
        }

        if (flags & GeometryGeneratorFlags_Dynamic) {
            visitor.set_current_component(owner);
            c.generateDecorations(false, hints, st, dg_swap);
        }

        for (SimTK::DecorativeGeometry const& geom : dg_swap) {
            // this step populates the outparam with concerete geometry instances
            geom.implementGeometry(visitor);
        }
    }
}

void osmv::Renderer::draw(Application const& ui, State_geometry& geometry) {
    // overview:
    //
    // drawing the scene efficiently is a fairly involved process. I apologize for that, but
    // rendering scenes efficiently with OpenGL requires an appreciation of OpenGL, GPUs,
    // and designing APIs that are flexible (because devs inevitably will want to customize
    // draw calls) and compatible with OpenSim.
    //
    // this is a forward (as opposed to deferred) renderer that borrows some ideas from deferred
    // rendering techniques. It *mostly* draws the entire scene in one pass (forward rending) but
    // the rendering step *also* writes to a multi-render-target (MRT) FBO that extra information
    // such as what's currently selected, and it uses that information in downstream sampling steps
    // (kind of like how deferred rendering puts everything into information-dense buffers). The
    // reason this rendering pipeline isn't fully deferred (gbuffers, albeido, etc.) is because
    // the scene is lit by a single directional light and the shading is fairly simple - there's
    // no perf upside to deferred shading in that particular scenario.

    std::vector<Mesh_instance>& meshes = geometry.meshes;

    // step 1: partition the mesh instances into those that are solid and those that require
    //         alpha blending
    //
    // ideally, rendering would follow the `painter's algorithm` and draw everything back-to-front.
    // We don't do that here, because constructing the various octrees, bsp's etc. to do that would
    // add a bunch of complexity CPU-side that's entirely unnecessary for such basic scenes. Also,
    // OpenGL benefits from the entirely opposite algorithm (render front-to-back) because it
    // uses depth testing as part of the "early fragment test" phase. Isn't low-level fun? :)
    //
    // so the hack here is to indiscriminately render all solid geometry first followed by
    // indiscriminately rendering all alpha-blended geometry. The edge-case failure here is that
    // alpha blended geometry, itself, should be rendered back-to-front because alpha-blended
    // geometry can be intercalated or occluding other alpha-blended geometry.
    auto has_solid_color = [](Mesh_instance const& a) { return a.rgba.a >= 1.0f; };
    std::partition(meshes.begin(), meshes.end(), has_solid_color);

    // step 2: precompute any matrices
    glm::mat4 view_mtx = compute_view_matrix(theta, phi, radius, pan);
    glm::mat4 proj_mtx = glm::perspective(fov, ui.window_aspect_ratio(), znear, zfar);
    glm::vec3 view_pos = spherical_2_cartesian(theta, phi, radius);

    // step 3: bind to an off-screen framebuffer object (FBO)
    //
    // drawing into this FBO writes to textures that the user can't see, but that can
    // be sampled by downstream shaders
    GLint original_draw_fbo;
    GLint original_read_fbo;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &original_draw_fbo);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &original_read_fbo);
    gl::BindFrameBuffer(GL_FRAMEBUFFER, state->buffers.gMRT_fbo);

    // step 4: clear the FBO for a new draw call
    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::DrawBuffers(GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // step 5: render the scene to the FBO using a multiple-render-target (MRT)
    //         multisampled (MSXAAed) shader. FBO outputs are:
    //
    // - COLOR0: main target: multisampled scene geometry
    //
    //     - contains OpenSim geometry rendered /w Gouraud shading this is *mostly* what the
    //       user actually sees
    //
    // - COLOR1: selection logic target: single-sampled ID encodings
    //
    //     - 8 bit unsigned byte per channel, 32-bit buffer (rgba)
    //
    //     - RGB: 24-bit (little-endian) ID of the drawn element
    //
    //     - A: current selection state, where:
    //         - 0.0f: not selected
    //         - 1.0f: selected
    //
    //     - The user does not directly see this buffer. It's used in subsequent steps to
    //       rim-highlight geometry and figure out what element the mouse is over without
    //       needing to do any work in the CPU (e.g. bounding box checks, ray traces)
    if (true) {
        glPolygonMode(GL_FRONT_AND_BACK, wireframe_mode ? GL_LINE : GL_FILL);

        // draw state geometry
        Gouraud_mrt_shader& shader = state->shaders.gouraud;
        gl::UseProgram(shader.program);

        gl::Uniform(shader.uProjMat, proj_mtx);
        gl::Uniform(shader.uViewMat, view_mtx);
        gl::Uniform(shader.uLightPos, light_pos);
        gl::Uniform(shader.uLightColor, light_rgb);
        gl::Uniform(shader.uViewPos, view_pos);

        for (size_t instance_idx = 0; instance_idx < meshes.size(); ++instance_idx) {
            Mesh_instance const& m = meshes[instance_idx];

            // COLOR1: will receive selection logic via the RGBA channels
            //
            //     - RGB (24 bits): little-endian encoded index+1 of the geometry instance
            //     - A   (8 bits): whether the geometry instance is currently selected or not
            assert(instance_idx < std::numeric_limits<uint32_t>::max());
            uint32_t color_id = static_cast<uint32_t>(instance_idx + 1);
            float r = static_cast<float>(color_id & 0xff) / 255.0f;
            float g = static_cast<float>((color_id >> 8) & 0xff) / 255.0f;
            float b = static_cast<float>((color_id >> 16) & 0xff) / 255.0f;
            float a = m._rim_alpha;

            gl::Uniform(shader.uRgba2, glm::vec4{r, g, b, a});
            gl::Uniform(shader.uRgba, m.rgba);
            gl::Uniform(shader.uModelMat, m.transform);
            gl::Uniform(shader.uNormalMat, m.normal_xform);

            Mesh_on_gpu& md = global_mesh_lookup(m._meshid);
            gl::BindVertexArray(md.main_vao);
            gl::DrawArrays(GL_TRIANGLES, 0, md.sizei());
        }
        gl::BindVertexArray();

        // (optional): draw a chequered floor
        //
        // only drawn to COLOR0, because it doesn't contribute to selection logic etc.
        if (show_floor) {
            Plain_texture_shader& pts = state->shaders.plain_texture;
            gl::DrawBuffers(GL_COLOR_ATTACHMENT0);
            gl::UseProgram(pts.p);

            gl::Uniform(pts.projMat, proj_mtx);
            gl::Uniform(pts.viewMat, view_mtx);
            gl::Uniform(pts.modelMat, state->floor.model_mtx);
            gl::ActiveTexture(GL_TEXTURE0);
            gl::BindTexture(state->floor.floor_texture);
            gl::Uniform(pts.uSampler0, gl::texture_index<GL_TEXTURE0>());

            gl::BindVertexArray(state->floor.vao);
            gl::DrawArrays(GL_TRIANGLES, 0, state->floor.vbo.sizei());
            gl::BindVertexArray();
        }

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // (optional): render scene normals
        //
        // if the caller wants to view normals, pump the scene through a specialized shader
        // that draws normals as lines in COLOR0
        if (show_mesh_normals) {
            Normals_shader& ns = state->shaders.normals;
            gl::DrawBuffers(GL_COLOR_ATTACHMENT0);

            gl::UseProgram(ns.program);
            gl::Uniform(ns.uProjMat, proj_mtx);
            gl::Uniform(ns.uViewMat, view_mtx);

            for (Mesh_instance const& m : meshes) {
                gl::Uniform(ns.uModelMat, m.transform);
                gl::Uniform(ns.uNormalMat, m.normal_xform);

                Mesh_on_gpu& md = global_mesh_lookup(m._meshid);
                gl::BindVertexArray(md.normal_vao);
                gl::DrawArrays(GL_TRIANGLES, 0, md.sizei());
            }
            gl::BindVertexArray();
        }
    }

    // step 6: figure out if the mouse is hovering over anything
    //
    // in the previous draw call, COLOR1's RGB channels encoded the index of the mesh instance.
    // Extracting that pixel value (without MSXAA blending) and decoding it back into an index
    // makes it possible to figure out what OpenSim::Component the mouse is over without requiring
    // complex spatial algorithms
    if (true) {
        // bind to a non-MSXAAed texture
        gl::BindFrameBuffer(GL_FRAMEBUFFER, state->buffers.gSkipMSXAA_fbo);

        // blit COLOR1 to the non-MSXAAed FBO
        //
        // by skipping MSXAA, every value in this output should to be exactly the same as the
        // value provided during drawing. Sampling the color with MSXAA could potentially blend
        // adjacent values together, resulting in junk.
        Skip_msxaa_blitter_shader& shader = state->shaders.skip_msxaa_shader;
        gl::UseProgram(shader.p);
        gl::Uniform(shader.uModelMat, gl::identity_val);
        gl::Uniform(shader.uViewMat, gl::identity_val);
        gl::Uniform(shader.uProjMat, gl::identity_val);
        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(state->buffers.gColor1_mstex);
        gl::Uniform(shader.uSampler0, gl::texture_index<GL_TEXTURE0>());
        gl::BindVertexArray(state->skip_msxaa_quad_vao);
        gl::DrawArrays(GL_TRIANGLES, 0, state->quad_vbo.sizei());
        gl::BindVertexArray();

        // figure out where the mouse is
        //
        // - SDL screen coords are traditional screen coords. Origin top-left, Y goes down
        // - OpenGL screen coords are mathematical coords. Origin bottom-left, Y goes up
        sdl::Mouse_state m = sdl::GetMouseState();
        sdl::Window_dimensions d = ui.window_dimensions();
        GLsizei xbl = m.x;
        GLsizei ybl = d.h - m.y;

        // read the pixel under the mouse
        //
        // - you *could* just read the value directly from the FBO with `glReadPixels`, which is
        //   what the first iteration of this alg. did
        //
        // - However, that glReadPixels call will cost *A LOT*. On my machine (Ryzen1600 /w
        //   Geforce 1060), it costs around 30 % FPS (300 FPS --> 200 FPS)
        //
        // - This isn't because the transfer is expensive--it's just a single pixel, after all--but
        //   because reading the pixel forces the OpenGL driver to flush all pending rendering
        //   operations to the FBO (known as a "pipeline stall")
        //
        // - If you don't believe me, set `fast_mode` to `false` below
        //
        // - So this algorithm uses a crafty trick, which is to use two pixel buffer objects (PBOs)
        //   to asynchronously transfer the pixel *from the previous frame* into CPU memory using
        //   asynchronous DMA. The trick uses two PBOs, which each of which are either:
        //
        //   1. Requesting the pixel value (via glReadPixel). The OpenGL spec does *not* require
        //      that the PBO is populated once `glReadPixel` returns, so this does not cause a
        //      pipeline stall
        //
        //   2. Mapping the PBO that requested a pixel value **on the last frame**. The OpenGL spec
        //      requires that this PBO is populated once the mapping is enabled, so this will
        //      stall the pipeline. However, that pipeline stall will be on the *previous* frame
        //      which is less costly to stall on

        static constexpr bool fast_mode = true;

        uint32_t color_id = 0;
        if (fast_mode) {
            int reader = state->buffers.pbo_idx % 2;
            int mapper = (state->buffers.pbo_idx + 1) % 2;

            // launch asynchronous request for this frame's pixel
            gl::BindBuffer(state->buffers.pbos[static_cast<size_t>(reader)]);
            glReadPixels(xbl, ybl, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

            // synchrnously read last frame's pixel
            gl::BindBuffer(state->buffers.pbos[static_cast<size_t>(mapper)]);
            GLubyte* src = static_cast<GLubyte*>(glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY));

            color_id = src[0];
            color_id |= static_cast<uint32_t>(src[1]) << 8;
            color_id |= static_cast<uint32_t>(src[2]) << 16;

            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);

            // flip buffers
            state->buffers.pbo_idx = (state->buffers.pbo_idx + 1) % 2;
        } else {
            // slow mode: synchronously read the current frame's pixel under the cursor

            GLubyte rgba[4]{};
            glReadPixels(xbl, ybl, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
            color_id = rgba[0];
            color_id |= static_cast<uint32_t>(rgba[1]) << 8;
            color_id |= static_cast<uint32_t>(rgba[2]) << 16;
        }

        // the decoded value is the index + 1, which we hold as the selected value because
        // +1 has the handy property of making 0 into a senteniel for "nothing selected"
        if (color_id == 0) {
            hovered_component = nullptr;
        } else {
            size_t instance_idx = color_id - 1;
            OpenSim::Component const* hovered = meshes[instance_idx].owner;
            hovered_component = hovered;
        }
    }

    // step 7: resolve MSXAA
    //
    // Resolve the MSXAA samples in COLOR0 and COLOR1 non-MSXAAed textures. This is done separately
    // because an intermediate step (decoding pixel colors into Component indices) cannot work with
    // post-resolved data (we need to *guarantee* that colors in the buffers are not blended if
    // they contain non-blendable information, like indices).
    {
        auto d = state->buffers.dims;
        int w = d.w;
        int h = d.h;

        // blit COLOR0
        glBindFramebuffer(GL_READ_FRAMEBUFFER, state->buffers.gMRT_fbo);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, state->buffers.gColor0_resolved_fbo);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        gl::BlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        // blit COLOR1
        glBindFramebuffer(GL_READ_FRAMEBUFFER, state->buffers.gMRT_fbo);
        glReadBuffer(GL_COLOR_ATTACHMENT1);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, state->buffers.gColor1_resolved_fbo);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        gl::BlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    // step 8: compose final render and write it to the output FBO
    {
        // ensure the output is written to the output FBO
        glBindFramebuffer(GL_READ_FRAMEBUFFER, original_read_fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, original_draw_fbo);

        // draw the edges over the rendered scene
        Edge_detection_shader& shader = state->shaders.edge_detection_shader;
        gl::UseProgram(shader.p);

        // setup draw call to draw a quad accross the entire screen
        gl::Uniform(shader.uProjMat, gl::identity_val);
        gl::Uniform(shader.uViewMat, gl::identity_val);
        gl::Uniform(shader.uModelMat, gl::identity_val);
        gl::Uniform(shader.uBackgroundColor, background_rgba);

        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(state->buffers.gColor0_resolved);
        gl::Uniform(shader.uSamplerSceneColors, gl::texture_index<GL_TEXTURE0>());

        gl::ActiveTexture(GL_TEXTURE1);
        gl::BindTexture(state->buffers.gColor1_resolved);
        gl::Uniform(shader.uSamplerSelectionEdges, gl::texture_index<GL_TEXTURE1>());

        gl::BindVertexArray(state->edge_detection_quad_vao);
        gl::DrawArrays(GL_TRIANGLES, 0, state->quad_vbo.sizei());
        gl::BindVertexArray();
    }

    // (optional): render debug quads
    //
    // if the application is rendering in debug mode, then render quads for the intermediate
    // buffers (selection etc.) because it's handy for debugging
    if (ui.is_in_debug_mode()) {
        Plain_texture_shader& pts = state->shaders.plain_texture;
        gl::UseProgram(pts.p);

        gl::Uniform(pts.projMat, gl::identity_val);
        gl::Uniform(pts.viewMat, gl::identity_val);
        gl::BindVertexArray(state->pts_quad_vao);

        glm::mat4 row1 = []() {
            glm::mat4 m = glm::identity<glm::mat4>();
            m = glm::translate(m, glm::vec3{0.80f, 0.80f, -1.0f});  // move to [+0.6, +1.0]
            m = glm::scale(m, glm::vec3{0.20f});  // so it becomes [-0.2, +0.2]
            return m;
        }();

        gl::Uniform(pts.modelMat, row1);
        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(state->buffers.gColor0_resolved);
        gl::Uniform(pts.uSampler0, gl::texture_index<GL_TEXTURE0>());
        gl::DrawArrays(GL_TRIANGLES, 0, state->quad_vbo.sizei());

        glm::mat4 row2 = []() {
            glm::mat4 m = glm::identity<glm::mat4>();
            m = glm::translate(m, glm::vec3{0.80f, 0.40f, -1.0f});  // move to [+0.6, +1.0] in x
            m = glm::scale(m, glm::vec3{0.20f});  // so it becomes [-0.2, +0.2]
            return m;
        }();

        gl::Uniform(pts.modelMat, row2);
        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(state->buffers.gColor1_resolved);
        gl::Uniform(pts.uSampler0, gl::texture_index<GL_TEXTURE0>());
        gl::DrawArrays(GL_TRIANGLES, 0, state->quad_vbo.sizei());

        gl::BindVertexArray();
    }
}

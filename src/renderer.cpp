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

// OpenGL stuff (shaders, whatever else)
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
        gl::Uniform_sampler2DMS uSamplerSceneColors = gl::GetUniformLocation(p, "uSamplerSceneColors");
        gl::Uniform_sampler2DMS uSamplerSelectionEdges = gl::GetUniformLocation(p, "uSamplerSelectionEdges");
        gl::Uniform_int uNumMultisamples = gl::GetUniformLocation(p, "uNumMultisamples");

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
}

using namespace SimTK;
using namespace OpenSim;

// OpenSim rendering specifics
namespace {
    // mesh IDs are guaranteed to be globally unique and monotonically increase from
    // 1
    //
    // this guarantee is important because it means that calling code can use direct
    // integer index lookups, rather than having to rely on (e.g.) a runtime
    // hashtable
    using Mesh_id = size_t;
    using Mesh_ctor_fn = void(std::vector<osmv::Untextured_vert>&);
    static constexpr std::array<Mesh_ctor_fn*, 3> reserved_mesh_ctors = {
        osmv::unit_sphere_triangles, osmv::simbody_cylinder_triangles, osmv::simbody_brick_triangles};
    static constexpr Mesh_id sphere_meshid = 0;
    static constexpr Mesh_id cylinder_meshid = 1;
    static constexpr Mesh_id cube_meshid = 2;
    static constexpr size_t num_reserved_meshes = 3;
    static_assert(
        num_reserved_meshes == reserved_mesh_ctors.size(),
        "if this fails, you might be missing a constructor function for a reserved mesh");

    static std::mutex g_mesh_cache_mutex;
    static std::unordered_map<std::string, std::vector<osmv::Untextured_vert>> g_mesh_cache;

    // one instance of a mesh
    //
    // a model may contain multiple instances of the same mesh
    struct Mesh_instance final {
        Component const* owner;

        glm::mat4 transform;
        glm::mat4 normal_xform;
        glm::vec4 rgba;

        Mesh_id mesh_id;
    };

    // for this API, an OpenSim model's geometry is described as a sequence of
    // rgba-colored mesh instances that are transformed into world coordinates
    struct State_geometry final {
        std::vector<Mesh_instance> mesh_instances;

        void clear() {
            mesh_instances.clear();
        }
    };

    struct Geometry_loader final {
        // this swap space prevents the geometry loader from having to allocate
        // space every time geometry is requested
        // Array_<DecorativeGeometry> decorative_geom_swap;
        SimTK::PolygonalMesh pm_swap;
        Array_<SimTK::DecorativeGeometry> dg_swp;

        // two-way lookup to establish a meshid-to-path mappings. This is so
        // that the renderer can just opaquely handle ID ints
        std::vector<std::string> meshid_to_str = std::vector<std::string>(num_reserved_meshes);
        std::unordered_map<std::string, Mesh_id> str_to_meshid;

        void all_geometry_in(OpenSim::Model const& model, SimTK::State const& s, State_geometry& out);
        void load_mesh(Mesh_id id, std::vector<osmv::Untextured_vert>& out);
    };

    // OpenSim mesh shown in main window
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

    // create an xform that transforms the unit cylinder into a line between
    // two points
    static glm::mat4 cylinder_to_line_xform(float line_width, glm::vec3 const& p1, glm::vec3 const& p2) {
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

    // load a SimTK PolygonalMesh into a more generic Untextured_mesh struct
    static void load_mesh_data(PolygonalMesh const& mesh, std::vector<osmv::Untextured_vert>& triangles) {

        // helper function: gets a vertex for a face
        auto get_face_vert_pos = [&](int face, int vert) {
            SimTK::Vec3 pos = mesh.getVertexPosition(mesh.getFaceVertex(face, vert));
            return glm::vec3{pos[0], pos[1], pos[2]};
        };

        auto make_normal = [](glm::vec3 const& p1, glm::vec3 const& p2, glm::vec3 const& p3) {
            return glm::cross(p2 - p1, p3 - p1);
        };

        triangles.clear();

        for (auto face = 0; face < mesh.getNumFaces(); ++face) {
            auto num_vertices = mesh.getNumVerticesForFace(face);

            if (num_vertices < 3) {
                // line?: ignore
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

    struct Geometry_visitor final : public DecorativeGeometryImplementation {
        OpenSim::Model const& model;
        SimTK::State const& state;
        Geometry_loader& impl;
        State_geometry& out;
        Component const* current_component = nullptr;

        Geometry_visitor(
            OpenSim::Model const& _model, SimTK::State const& _state, Geometry_loader& _impl, State_geometry& _out) :
            model{_model},
            state{_state},
            impl{_impl},
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
        }
        void implementLineGeometry(const DecorativeLine& geom) override {
            // a line is essentially a thin cylinder that connects two points
            // in space. This code eagerly performs that transformation

            glm::mat4 xform = transform(geom);
            glm::vec3 p1 = xform * to_vec4(geom.getPoint1());
            glm::vec3 p2 = xform * to_vec4(geom.getPoint2());

            glm::mat4 cylinder_xform = cylinder_to_line_xform(0.005f, p1, p2);
            glm::mat4 normal_mtx = glm::transpose(glm::inverse(cylinder_xform));

            out.mesh_instances.push_back({current_component, cylinder_xform, normal_mtx, rgba(geom), cylinder_meshid});
        }
        void implementBrickGeometry(const DecorativeBrick& geom) override {
            SimTK::Vec3 dims = geom.getHalfLengths();
            glm::mat4 xform = glm::scale(transform(geom), glm::vec3{dims[0], dims[1], dims[2]});
            glm::mat4 normal_mtx = glm::transpose(glm::inverse(xform));

            out.mesh_instances.push_back({current_component, xform, normal_mtx, rgba(geom), cube_meshid});
        }
        void implementCylinderGeometry(const DecorativeCylinder& geom) override {
            glm::mat4 m = transform(geom);
            glm::vec3 s = scale_factors(geom);
            s.x *= static_cast<float>(geom.getRadius());
            s.y *= static_cast<float>(geom.getHalfHeight());
            s.z *= static_cast<float>(geom.getRadius());

            glm::mat4 xform = glm::scale(m, s);
            glm::mat4 normal_mtx = glm::transpose(glm::inverse(xform));

            out.mesh_instances.push_back({current_component, xform, normal_mtx, rgba(geom), cylinder_meshid});
        }
        void implementCircleGeometry(const DecorativeCircle&) override {
        }
        void implementSphereGeometry(const DecorativeSphere& geom) override {
            float r = static_cast<float>(geom.getRadius());
            glm::mat4 xform = glm::scale(transform(geom), glm::vec3{r, r, r});
            glm::mat4 normal_mtx = glm::transpose(glm::inverse(xform));

            out.mesh_instances.push_back({current_component, xform, normal_mtx, rgba(geom), sphere_meshid});
        }
        void implementEllipsoidGeometry(const DecorativeEllipsoid&) override {
        }
        void implementFrameGeometry(const DecorativeFrame&) override {
        }
        void implementTextGeometry(const DecorativeText&) override {
        }
        void implementMeshGeometry(const DecorativeMesh&) override {
        }
        void implementMeshFileGeometry(const DecorativeMeshFile& m) override {
            auto xform = glm::scale(transform(m), scale_factors(m));
            std::string const& path = m.getMeshFile();

            // HACK: globally cache the loaded meshes
            //
            // this is here because OpenSim "helpfully" pre-loads the fucking
            // meshes in the main thread, so the loading code is currently
            // redundantly re-loading the meshes that OpenSim loads
            //
            // this will be fixed once the Model crawling is customized to step
            // around OpenSim's shitty implementation (which can't be easily
            // multithreaded)
            {
                std::lock_guard l{g_mesh_cache_mutex};
                auto [it, inserted] =
                    g_mesh_cache.emplace(std::piecewise_construct, std::forward_as_tuple(path), std::make_tuple());

                if (inserted) {
                    load_mesh_data(m.getMesh(), it->second);
                }
            }

            // load the path into the model-to-path mapping lookup
            Mesh_id meshid = [this, &path]() {
                auto [it, inserted] = impl.str_to_meshid.emplace(
                    std::piecewise_construct, std::forward_as_tuple(path), std::make_tuple());

                if (not inserted) {
                    return it->second;
                } else {
                    // populate lookups

                    size_t meshid_s = impl.meshid_to_str.size();
                    assert(meshid_s < std::numeric_limits<Mesh_id>::max());
                    Mesh_id rv = static_cast<Mesh_id>(meshid_s);
                    impl.meshid_to_str.push_back(path);
                    it->second = rv;
                    return rv;
                }
            }();

            glm::mat4 normal_mtx = glm::transpose(glm::inverse(xform));

            out.mesh_instances.push_back({current_component, xform, normal_mtx, rgba(m), meshid});
        }
        void implementArrowGeometry(const DecorativeArrow&) override {
        }
        void implementTorusGeometry(const DecorativeTorus&) override {
        }
        void implementConeGeometry(const DecorativeCone&) override {
        }
    };

    void Geometry_loader::all_geometry_in(OpenSim::Model const& m, SimTK::State const& s, State_geometry& out) {
        // iterate over all components in the OpenSim model, keeping a few things in mind:
        //
        // - Anything in the component tree *might* render geometry
        //
        // - For selection logic, we only (currently) care about certain high-level components,
        //   like muscles
        //
        // - Pretend the component tree traversal is implementation-defined because it's a bit of
        //   a mess. At the moment it looks like it's a breadth-first recursive descent
        //
        // - Components of interest, like muscles, might not render their geometry - it might be
        //   delegated to a subcomponent
        //
        // So this algorithm assumes that the list iterator is arbitrary, but always returns
        // *something* in a tree that has the current model as a root. So, for each component that
        // pops out of `getComponentList`, crawl "up" to the root. If we encounter something
        // interesting (e.g. a `Muscle`) then we tag the geometry against that component, rather
        // than the component that is rendering.

        auto visitor = Geometry_visitor{m, s, *this, out};
        for (Component const& c : m.getComponentList()) {

            Component const* owner = nullptr;
            for (Component const* p = &c; p != &m; p = &p->getOwner()) {
                if (dynamic_cast<OpenSim::Muscle const*>(p)) {
                    owner = p;
                    break;
                }
            }

            dg_swp.clear();
            visitor.set_current_component(nullptr);
            c.generateDecorations(true, m.getDisplayHints(), s, dg_swp);
            visitor.set_current_component(owner);
            c.generateDecorations(false, m.getDisplayHints(), s, dg_swp);

            for (SimTK::DecorativeGeometry const& geom : dg_swp) {
                geom.implementGeometry(visitor);
            }
        }
    }

    void Geometry_loader::load_mesh(Mesh_id id, std::vector<osmv::Untextured_vert>& out) {
        // handle reserved mesh IDs: they are generated from a constructor function populated
        // above
        if (id < num_reserved_meshes) {
            reserved_mesh_ctors[id](out);
            return;
        }

        std::string const& path = meshid_to_str.at(id);

        std::lock_guard l{g_mesh_cache_mutex};

        auto [it, inserted] =
            g_mesh_cache.emplace(std::piecewise_construct, std::forward_as_tuple(path), std::make_tuple());

        if (inserted) {
            // wasn't cached: load the mesh

            PolygonalMesh& mesh = pm_swap;
            mesh.clear();
            mesh.loadFile(path);

            load_mesh_data(mesh, it->second);
        }

        out = it->second;
    }

    static gl::Pixel_pack_buffer make_single_pixel_PBO() {
        gl::Pixel_pack_buffer rv;
        gl::BindBuffer(rv);
        GLubyte rgba[4]{};  // initialize to zeroed values
        gl::BufferData(rv.type, 4, rgba, GL_STREAM_READ);
        gl::UnbindBuffer(rv);
        return rv;
    }

    // renderer MRT FBO:
    //
    // - COLOR0: multisampled texture containing scene geometry
    // - DEPTH_STENCIL_ATTACHMENT: depth+stencil buffers
    //
    // this renderer first draws into these textures, which are used for
    // runtime calulations (e.g. selection logic) and then blitted onto
    // the screen via a quad.

    struct Renderer_buffers {
        sdl::Window_dimensions dims;
        int samples;

        // stores rendered scenery
        gl::Texture_2d_multisample gColor0_mstex;

        // stores selected item index and selection rim alphas
        gl::Texture_2d_multisample gColor1_mstex;

        // depth + stencil RBO: needed to "complete" the FBO
        gl::Render_buffer gDepth24Stencil8_rbo;

        // MRT FBO that writes scenery + selection info in one pass
        gl::Frame_buffer gMRT_fbo;

        // texture+fbo for sampling non-MSXAAed version of the scene
        //
        // used to sample out the currently-moused-over target
        gl::Texture_2d gSkipMSXAA_tex;
        gl::Frame_buffer gSkipMSXAA_fbo;

        // PBOs for asynchronously reading the non-MSXAAed value for
        // the mouseover target
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
                // save original FBO
                GLint original_draw_fbo;
                GLint original_read_fbo;
                glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &original_draw_fbo);
                glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &original_read_fbo);

                gl::Frame_buffer rv;

                // configure main FBO
                gl::BindFrameBuffer(GL_FRAMEBUFFER, rv);
                gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gColor0_mstex.type, gColor0_mstex, 0);
                gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, gColor1_mstex.type, gColor1_mstex, 0);
                gl::FramebufferRenderbuffer(
                    GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gDepth24Stencil8_rbo);

                // main FBO allocated, check it's OK
                assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

                // restore original FBO
                gl::BindFrameBuffer(GL_DRAW_FRAMEBUFFER, original_draw_fbo);
                gl::BindFrameBuffer(GL_READ_FRAMEBUFFER, original_read_fbo);

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
                // save original FBO
                GLint original_draw_fbo;
                GLint original_read_fbo;
                glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &original_draw_fbo);
                glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &original_read_fbo);

                gl::Frame_buffer rv;

                // configure non-MSXAA fbo
                gl::BindFrameBuffer(GL_FRAMEBUFFER, rv);
                gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gSkipMSXAA_tex.type, gSkipMSXAA_tex, 0);

                // check non-MSXAA OK
                assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

                // restore original FBO
                gl::BindFrameBuffer(GL_DRAW_FRAMEBUFFER, original_draw_fbo);
                gl::BindFrameBuffer(GL_READ_FRAMEBUFFER, original_read_fbo);

                return rv;
            }()} {
        }
    };
}

// osmv::Renderer implementation

namespace osmv {
    struct Renderer_private_state final {
        // OpenGL shaders
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

        // floor texture details
        struct {
            gl::Array_bufferT<osmv::Shaded_textured_vert> vbo = []() {
                auto copy = osmv::shaded_textured_quad_verts;
                for (osmv::Shaded_textured_vert& st : copy) {
                    st.texcoord *= 50.0f;  // make chequers smaller
                }
                return gl::Array_bufferT<osmv::Shaded_textured_vert>{copy};
            }();

            gl::Vertex_array vao = Plain_texture_shader::create_vao(vbo);
            gl::Texture_2d floor_texture = osmv::generate_chequered_floor_texture();
            glm::mat4 model_mtx = glm::scale(
                glm::rotate(glm::identity<glm::mat4>(), osmv::pi_f / 2, {-1.0, 0.0, 0.0}), {100.0f, 100.0f, 0.0f});
        } floor;

        // temporary scratch space
        struct {
            std::vector<Untextured_vert> mesh;
            State_geometry geom;
        } scratch;

        // opensim geometry loading
        struct {
            Geometry_loader geom_loader;

            // indexed by meshid
            std::vector<std::optional<Mesh_on_gpu>> meshes;
        } osim;

        Renderer_buffers buffers;

        Renderer_private_state(Application& app) : buffers{app.window_dimensions(), app.samples()} {
        }

        void load_geom_from_model(OpenSim::Model const& model, SimTK::State const& s) {
            // TODO: this step should be exposed to callers, ideally, because it's quite
            // slow

            scratch.geom.clear();
            osim.geom_loader.all_geometry_in(model, s, scratch.geom);

            for (Mesh_instance const& mi : scratch.geom.mesh_instances) {
                if (osim.meshes.size() >= (mi.mesh_id + 1) and osim.meshes[mi.mesh_id].has_value()) {
                    // the mesh is already loaded
                    continue;
                }

                osim.geom_loader.load_mesh(mi.mesh_id, scratch.mesh);
                osim.meshes.resize(std::max(osim.meshes.size(), mi.mesh_id + 1));
                osim.meshes[mi.mesh_id] = Mesh_on_gpu{scratch.mesh};
            }

            // HACK: the scene might contain blended (alpha < 1.0f) elements. These must
            // be drawn last, ideally back-to-front (not yet implemented); otherwise,
            // OpenGL will early-discard after the vertex shader when they fail a
            // depth test
            std::sort(
                scratch.geom.mesh_instances.begin(),
                scratch.geom.mesh_instances.end(),
                [](Mesh_instance const& a, Mesh_instance const& b) { return a.rgba.a > b.rgba.a; });
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
osmv::Renderer::Renderer(osmv::Application& app) : state(new Renderer_private_state(app)) {
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
            theta += 2.0f * static_cast<float>(M_PI) * sensitivity * dx;
            phi += 2.0f * static_cast<float>(M_PI) * sensitivity * dy;
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
            radius *= wheel_sensitivity;
        }

        if (e.wheel.y <= 0 and radius < 100.0f) {
            radius /= wheel_sensitivity;
        }

        return Event_response::handled;
    }

    return Event_response::ignored;
}

template<typename V>
static V& asserting_find(std::vector<std::optional<V>>& meshes, Mesh_id id) {
    assert(id + 1 <= meshes.size());
    assert(meshes[id]);
    return meshes[id].value();
}

static glm::mat4 compute_view_matrix(float theta, float phi, float radius, glm::vec3 pan) {
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

static glm::vec3 spherical_2_cartesian(float theta, float phi, float radius) {
    return glm::vec3{radius * sinf(theta) * cosf(phi), radius * sinf(phi), radius * cosf(theta) * cosf(phi)};
}

void osmv::Renderer::draw(
    Application const& ui, OpenSim::Model const& model, SimTK::State const& s, OpenSim::Component const* selected) {

    // load model geometry
    state->load_geom_from_model(model, s);

    glm::mat4 view_mtx = compute_view_matrix(theta, phi, radius, pan);
    glm::mat4 proj_mtx = glm::perspective(fov, ui.window_aspect_ratio(), znear, zfar);
    glm::vec3 view_pos = spherical_2_cartesian(theta, phi, radius);

    // overview:
    //
    // this is a forward (as opposed to deferred) renderer that borrows some ideas from deferred
    // rendering techniques
    //
    // main motivator here is to render all scene geometry in a single pass, followed by using data
    // produced in that pass in subsequent shaders. This means that some things are over-calculated
    // (e.g. edge-detection for rim highlights are screen-wide), but all computation happens in
    // parallel on the GPU, rather than stalling on the CPU

    // step 1: bind to an off-screen FBO
    //
    // - this FBO writes to textures that can be sampled downstream
    GLint original_draw_fbo;
    GLint original_read_fbo;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &original_draw_fbo);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &original_read_fbo);

    gl::BindFrameBuffer(GL_FRAMEBUFFER, state->buffers.gMRT_fbo);
    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // step 1: render the scene to the off-screen textures using a multiple-render-target
    //         (MRT) multisampled (MSXAAed) shader
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

        // draw the floor
        if (show_floor) {
            Plain_texture_shader& shader = state->shaders.plain_texture;
            gl::DrawBuffers(GL_COLOR_ATTACHMENT0);
            gl::UseProgram(shader.p);

            gl::Uniform(shader.projMat, proj_mtx);
            gl::Uniform(shader.viewMat, view_mtx);
            gl::Uniform(shader.modelMat, state->floor.model_mtx);
            gl::ActiveTexture(GL_TEXTURE0);
            gl::BindTexture(state->floor.floor_texture);
            gl::Uniform(shader.uSampler0, gl::texture_index<GL_TEXTURE0>());

            gl::BindVertexArray(state->floor.vao);
            gl::DrawArrays(GL_TRIANGLES, 0, state->floor.vbo.sizei());
            gl::BindVertexArray();
        }

        // draw scene geometry
        Gouraud_mrt_shader& shader = state->shaders.gouraud;
        gl::DrawBuffers(GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1);
        gl::UseProgram(shader.program);

        gl::Uniform(shader.uProjMat, proj_mtx);
        gl::Uniform(shader.uViewMat, view_mtx);
        gl::Uniform(shader.uLightPos, light_pos);
        gl::Uniform(shader.uLightColor, light_color);
        gl::Uniform(shader.uViewPos, view_pos);

        std::vector<Mesh_instance> const& instances = state->scratch.geom.mesh_instances;
        for (size_t i = 0; i < instances.size(); ++i) {
            Mesh_instance const& m = instances[i];

            // COLOR1: will receive selection logic via the RGBA channels
            //
            //     - RGB (24 bits): little-endian encoded index+1 of the geometry instance
            //     - A   (8 bits): whether the geometry instance is currently selected or not
            uint32_t color_id = static_cast<uint32_t>(i + 1);
            float r = static_cast<float>(color_id & 0xff) / 255.0f;
            float g = static_cast<float>((color_id >> 8) & 0xff) / 255.0f;
            float b = static_cast<float>((color_id >> 16) & 0xff) / 255.0f;

            float a = 0.0f;
            if (m.owner != nullptr) {
                if (m.owner == selected) {
                    a = 1.0f;
                } else if (m.owner == hovered_component) {
                    a = 0.25f;
                }
            }

            gl::Uniform(shader.uRgba2, glm::vec4{r, g, b, a});
            gl::Uniform(shader.uRgba, m.rgba);
            gl::Uniform(shader.uModelMat, m.transform);
            gl::Uniform(shader.uNormalMat, m.normal_xform);

            Mesh_on_gpu& md = asserting_find(state->osim.meshes, m.mesh_id);
            gl::BindVertexArray(md.main_vao);
            gl::DrawArrays(GL_TRIANGLES, 0, md.sizei());
        }
        gl::BindVertexArray();

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

            for (auto& m : state->scratch.geom.mesh_instances) {
                gl::Uniform(ns.uModelMat, m.transform);
                gl::Uniform(ns.uNormalMat, m.normal_xform);

                Mesh_on_gpu& md = asserting_find(state->osim.meshes, m.mesh_id);
                gl::BindVertexArray(md.normal_vao);
                gl::DrawArrays(GL_TRIANGLES, 0, md.sizei());
            }
            gl::BindVertexArray();
        }
    }

    // step 2: figure out if the mouse is hovering over anything
    //
    // COLOR1's RGB channels encode the index of the mesh instance. Extracting that pixel value
    // (without MSXAA blending) and decoding it back into an index makes it possible to figure
    // out what the mouse is over
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

        uint32_t decoded = 0;
        if (fast_mode) {
            int reader = state->buffers.pbo_idx % 2;
            int mapper = (state->buffers.pbo_idx + 1) % 2;

            // launch asynchronous request for this frame's pixel
            gl::BindBuffer(state->buffers.pbos[static_cast<size_t>(reader)]);
            glReadPixels(xbl, ybl, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

            // synchrnously read last frame's pixel
            gl::BindBuffer(state->buffers.pbos[static_cast<size_t>(mapper)]);
            GLubyte* src = static_cast<GLubyte*>(glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY));

            decoded = src[0];
            decoded |= static_cast<uint32_t>(src[1]) << 8;
            decoded |= static_cast<uint32_t>(src[2]) << 16;

            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);

            // flip buffers
            state->buffers.pbo_idx = (state->buffers.pbo_idx + 1) % 2;
        } else {
            // slow mode: synchronously read the current frame's pixel under the cursor

            GLubyte rgba[4]{};
            glReadPixels(xbl, ybl, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
            decoded = rgba[0];
            decoded |= static_cast<uint32_t>(rgba[1]) << 8;
            decoded |= static_cast<uint32_t>(rgba[2]) << 16;
        }

        // the decoded value is the index + 1, which we hold as the selected value because
        // +1 has the handy property of making 0 into a senteniel for "nothing selected"
        if (decoded == 0) {
            hovered_component = nullptr;
        } else {
            hovered_component = state->scratch.geom.mesh_instances[decoded - 1].owner;
        }
    }

    // note: the scene geometry is now "done": COLOR0 is linked to an MSXAAed texture that can
    //       be blitted to the output

    // TODO: the MSXAAed textures should be pre-blitted, rather than running a funky custom
    // shader that handles it

    // step 3: compose the two textures together with a specialized shader and write
    //         to the output FBO
    //
    // - COLOR1: sampled as-is
    //
    // - COLOR2: passed through an edge-detection kernel to find the edges of all selected
    //           geometry in the scene
    //
    // - COMPOSITION: alpha-over blend COLOR2 over COLOR1 over background
    {
        // ensure the output is written to the output FBO
        glBindFramebuffer(GL_READ_FRAMEBUFFER, original_read_fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, original_draw_fbo);
        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // draw the edges over the rendered scene
        Edge_detection_shader& shader = state->shaders.edge_detection_shader;
        gl::UseProgram(shader.p);

        // screen-space quad
        gl::Uniform(shader.uProjMat, gl::identity_val);
        gl::Uniform(shader.uViewMat, gl::identity_val);
        gl::Uniform(shader.uModelMat, gl::identity_val);
        gl::Uniform(shader.uBackgroundColor, glm::vec4{0.99f, 0.98f, 0.96f, 1.0f});

        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(state->buffers.gColor0_mstex);
        gl::Uniform(shader.uSamplerSceneColors, gl::texture_index<GL_TEXTURE0>());

        gl::ActiveTexture(GL_TEXTURE1);
        gl::BindTexture(state->buffers.gColor1_mstex);
        gl::Uniform(shader.uSamplerSelectionEdges, gl::texture_index<GL_TEXTURE1>());

        gl::Uniform(shader.uNumMultisamples, ui.samples());

        glDisable(GL_DEPTH_TEST);
        gl::BindVertexArray(state->edge_detection_quad_vao);
        gl::DrawArrays(GL_TRIANGLES, 0, state->quad_vbo.sizei());
        gl::BindVertexArray();
        glEnable(GL_DEPTH_TEST);
    }
}

#include "src/3d/3d.hpp"

#include "src/config.hpp"
#include "src/utils/helpers.hpp"
#include "src/utils/sfinae.hpp"
#include "src/constants.hpp"
#include "src/3d/texturing.hpp"
#include "src/3d/gl_extensions.hpp"

#include <cstddef>

using namespace osc;
using namespace osc::todo;

// An instanced multi-render-target (MRT) shader that performes Gouraud shading for
// COLOR0 and RGB passthrough for COLOR1
//
// - COLOR0: geometry colored with Gouraud shading: i.e. "the scene"
// - COLOR1: RGB passthrough (selection logic + rim alphas)
struct osc::todo::Gouraud_mrt_shader final {
    gl::Program program = gl::CreateProgramFrom(
        gl::CompileFromSource<gl::Vertex_shader>(
            slurp_into_string(config::shader_path("gouraud_mrt.vert")).c_str()),
        gl::CompileFromSource<gl::Fragment_shader>(
            slurp_into_string(config::shader_path("gouraud_mrt.frag")).c_str()));

    // vertex attrs
    static constexpr gl::Attribute_vec3 aLocation{0};
    static constexpr gl::Attribute_vec3 aNormal{1};
    static constexpr gl::Attribute_vec2 aTexCoord{2};

    // instancing attrs
    static constexpr gl::Attribute_mat4x3 aModelMat{3};
    static constexpr gl::Attribute_mat3 aNormalMat{7};
    static constexpr gl::Attribute_vec4 aRgba0{10};
    static constexpr gl::Attribute_vec3 aRgb1{11};

    gl::Uniform_mat4 uProjMat = gl::GetUniformLocation(program, "uProjMat");
    gl::Uniform_mat4 uViewMat = gl::GetUniformLocation(program, "uViewMat");
    gl::Uniform_vec3 uLightPos = gl::GetUniformLocation(program, "uLightPos");
    gl::Uniform_vec3 uLightColor = gl::GetUniformLocation(program, "uLightColor");
    gl::Uniform_vec3 uViewPos = gl::GetUniformLocation(program, "uViewPos");
    gl::Uniform_bool uIsTextured = gl::GetUniformLocation(program, "uIsTextured");
    gl::Uniform_bool uIsShaded = gl::GetUniformLocation(program, "uIsShaded");
    gl::Uniform_sampler2d uSampler0 = gl::GetUniformLocation(program, "uSampler0");
    gl::Uniform_bool uSkipVP = gl::GetUniformLocation(program, "uSkipVP");

    template<typename Vbo, typename T = typename Vbo::value_type>
    static gl::Vertex_array create_vao(
        Vbo& vbo,
        gl::Typed_buffer_handle<GL_ELEMENT_ARRAY_BUFFER>& ebo,
        gl::Array_buffer<Mesh_instance, GL_DYNAMIC_DRAW>& instance_vbo) {

        static_assert(std::is_standard_layout<T>::value, "this is required for offsetof macro usage");

        gl::Vertex_array vao;

        gl::BindVertexArray(vao);
        gl::BindBuffer(vbo);
        gl::VertexAttribPointer(aLocation, false, sizeof(T), offsetof(T, pos));
        gl::EnableVertexAttribArray(aLocation);
        gl::VertexAttribPointer(aNormal, false, sizeof(T), offsetof(T, normal));
        gl::EnableVertexAttribArray(aNormal);

        if constexpr (has_texcoord<T>::value) {
            gl::VertexAttribPointer(aTexCoord, false, sizeof(T), offsetof(T, texcoord));
            gl::EnableVertexAttribArray(aTexCoord);
        }

        gl::BindBuffer(ebo.buffer_type, ebo);

        // set up instanced VBOs
        gl::BindBuffer(instance_vbo);

        gl::VertexAttribPointer(aModelMat, false, sizeof(Mesh_instance), offsetof(Mesh_instance, model_xform));
        gl::EnableVertexAttribArray(aModelMat);
        gl::VertexAttribDivisor(aModelMat, 1);

        gl::VertexAttribPointer(aNormalMat, false, sizeof(Mesh_instance), offsetof(Mesh_instance, normal_xform));
        gl::EnableVertexAttribArray(aNormalMat);
        gl::VertexAttribDivisor(aNormalMat, 1);

        // note: RGBs are tricksy, because their CPU-side data is UNSIGNED_BYTEs but
        // their GPU-side data is normalized FLOATs

        gl::VertexAttribPointer<decltype(aRgba0)::glsl_type, GL_UNSIGNED_BYTE>(
            aRgba0, true, sizeof(Mesh_instance), offsetof(Mesh_instance, rgba));
        gl::EnableVertexAttribArray(aRgba0);
        gl::VertexAttribDivisor(aRgba0, 1);

        gl::VertexAttribPointer<decltype(aRgb1)::glsl_type, GL_UNSIGNED_BYTE>(
            aRgb1, true, sizeof(Mesh_instance), offsetof(Mesh_instance, passthrough));
        gl::EnableVertexAttribArray(aRgb1);
        gl::VertexAttribDivisor(aRgb1, 1);

        gl::BindVertexArray();

        return vao;
    }
};

// A basic shader that just samples a texture onto the provided geometry
//
// useful for rendering quads etc.
struct osc::todo::Colormapped_plain_texture_shader final {
    gl::Program p = gl::CreateProgramFrom(
        gl::CompileFromSource<gl::Vertex_shader>(
            slurp_into_string(config::shader_path("colormapped_plain_texture.vert")).c_str()),
        gl::CompileFromSource<gl::Fragment_shader>(
            slurp_into_string(config::shader_path("colormapped_plain_texture.frag")).c_str()));

    static constexpr gl::Attribute_vec3 aPos{0};
    static constexpr gl::Attribute_vec2 aTexCoord{1};

    gl::Uniform_mat4 uMVP = gl::GetUniformLocation(p, "uMVP");
    gl::Uniform_sampler2d uSampler0 = gl::GetUniformLocation(p, "uSampler0");
    gl::Uniform_mat4 uSamplerMultiplier = gl::GetUniformLocation(p, "uSamplerMultiplier");

    template<typename Vbo, typename T = typename Vbo::value_type>
    static gl::Vertex_array create_vao(Vbo& vbo) {
        gl::Vertex_array vao;

        gl::BindVertexArray(vao);
        gl::BindBuffer(vbo);
        gl::VertexAttribPointer(aPos, false, sizeof(T), offsetof(T, pos));
        gl::EnableVertexAttribArray(aPos);
        gl::VertexAttribPointer(aTexCoord, false, sizeof(T), offsetof(T, texcoord));
        gl::EnableVertexAttribArray(aTexCoord);
        gl::BindVertexArray();

        return vao;
    }
};

struct osc::todo::Plain_texture_shader final {
    gl::Program p = gl::CreateProgramFrom(
        gl::CompileFromSource<gl::Vertex_shader>(
            slurp_into_string(config::shader_path("plain_texture.vert")).c_str()),
        gl::CompileFromSource<gl::Fragment_shader>(
            slurp_into_string(config::shader_path("plain_texture.frag")).c_str()));

    static constexpr gl::Attribute_vec3 aPos{0};
    static constexpr gl::Attribute_vec2 aTexCoord{1};

    gl::Uniform_mat4 uMVP = gl::GetUniformLocation(p, "uMVP");
    gl::Uniform_float uTextureScaler = gl::GetUniformLocation(p, "uTextureScaler");
    gl::Uniform_sampler2d uSampler0 = gl::GetUniformLocation(p, "uSampler0");

    template<typename Vbo, typename T = typename Vbo::value_type>
    static gl::Vertex_array create_vao(Vbo& vbo) {
        gl::Vertex_array vao;

        gl::BindVertexArray(vao);
        gl::BindBuffer(vbo);
        gl::VertexAttribPointer(aPos, false, sizeof(T), offsetof(T, pos));
        gl::EnableVertexAttribArray(aPos);
        gl::VertexAttribPointer(aTexCoord, false, sizeof(T), offsetof(T, texcoord));
        gl::EnableVertexAttribArray(aTexCoord);
        gl::BindVertexArray();

        return vao;
    }
};

// A specialized edge-detection shader for rim highlighting
struct osc::todo::Edge_detection_shader final {
    gl::Program p = gl::CreateProgramFrom(
        gl::CompileFromSource<gl::Vertex_shader>(
            slurp_into_string(config::shader_path("edge_detect.vert")).c_str()),
        gl::CompileFromSource<gl::Fragment_shader>(
            slurp_into_string(config::shader_path("edge_detect.frag")).c_str()));

    static constexpr gl::Attribute_vec3 aPos{0};
    static constexpr gl::Attribute_vec2 aTexCoord{1};

    gl::Uniform_mat4 uModelMat = gl::GetUniformLocation(p, "uModelMat");
    gl::Uniform_mat4 uViewMat = gl::GetUniformLocation(p, "uViewMat");
    gl::Uniform_mat4 uProjMat = gl::GetUniformLocation(p, "uProjMat");
    gl::Uniform_sampler2d uSampler0 = gl::GetUniformLocation(p, "uSampler0");
    gl::Uniform_vec4 uRimRgba = gl::GetUniformLocation(p, "uRimRgba");
    gl::Uniform_float uRimThickness = gl::GetUniformLocation(p, "uRimThickness");

    template<typename Vbo, typename T = typename Vbo::value_type>
    static gl::Vertex_array create_vao(Vbo& vbo) {
        gl::Vertex_array vao;

        gl::BindVertexArray(vao);
        gl::BindBuffer(vbo);
        gl::VertexAttribPointer(aPos, false, sizeof(T), offsetof(T, pos));
        gl::EnableVertexAttribArray(aPos);
        gl::VertexAttribPointer(aTexCoord, false, sizeof(T), offsetof(T, texcoord));
        gl::EnableVertexAttribArray(aTexCoord);
        gl::BindVertexArray();

        return vao;
    }
};

struct osc::todo::Skip_msxaa_blitter_shader final {
    gl::Program p = gl::CreateProgramFrom(
        gl::CompileFromSource<gl::Vertex_shader>(
            slurp_into_string(config::shader_path("skip_msxaa_blitter.vert")).c_str()),
        gl::CompileFromSource<gl::Fragment_shader>(
            slurp_into_string(config::shader_path("skip_msxaa_blitter.frag")).c_str()));

    static constexpr gl::Attribute_vec3 aPos{0};
    static constexpr gl::Attribute_vec2 aTexCoord{1};

    gl::Uniform_mat4 uModelMat = gl::GetUniformLocation(p, "uModelMat");
    gl::Uniform_mat4 uViewMat = gl::GetUniformLocation(p, "uViewMat");
    gl::Uniform_mat4 uProjMat = gl::GetUniformLocation(p, "uProjMat");
    gl::Uniform_sampler2DMS uSampler0 = gl::GetUniformLocation(p, "uSampler0");

    template<typename Vbo, typename T = typename Vbo::value_type>
    static gl::Vertex_array create_vao(Vbo& vbo) {
        gl::Vertex_array vao;

        gl::BindVertexArray(vao);
        gl::BindBuffer(vbo);
        gl::VertexAttribPointer(aPos, false, sizeof(T), offsetof(T, pos));
        gl::EnableVertexAttribArray(aPos);
        gl::VertexAttribPointer(aTexCoord, false, sizeof(T), offsetof(T, texcoord));
        gl::EnableVertexAttribArray(aTexCoord);
        gl::BindVertexArray();

        return vao;
    }
};

// uses a geometry shader to render normals as lines
struct osc::todo::Normals_shader final {
    gl::Program program = gl::CreateProgramFrom(
        gl::CompileFromSource<gl::Vertex_shader>(
            slurp_into_string(config::shader_path("draw_normals.vert")).c_str()),
        gl::CompileFromSource<gl::Fragment_shader>(
            slurp_into_string(config::shader_path("draw_normals.frag")).c_str()),
        gl::CompileFromSource<gl::Geometry_shader>(
            slurp_into_string(config::shader_path("draw_normals.geom")).c_str()));

    static constexpr gl::Attribute_vec3 aPos{0};
    static constexpr gl::Attribute_vec2 aNormal{1};

    gl::Uniform_mat4 uModelMat = gl::GetUniformLocation(program, "uModelMat");
    gl::Uniform_mat4 uViewMat = gl::GetUniformLocation(program, "uViewMat");
    gl::Uniform_mat4 uProjMat = gl::GetUniformLocation(program, "uProjMat");
    gl::Uniform_mat4 uNormalMat = gl::GetUniformLocation(program, "uNormalMat");

    template<typename Vbo, typename T = typename Vbo::value_type>
    static gl::Vertex_array create_vao(Vbo& vbo) {
        gl::Vertex_array vao;
        gl::BindVertexArray(vao);
        gl::BindBuffer(vbo);
        gl::VertexAttribPointer(aPos, false, sizeof(T), offsetof(T, pos));
        gl::EnableVertexAttribArray(aPos);
        gl::VertexAttribPointer(aNormal, false, sizeof(T), offsetof(T, normal));
        gl::EnableVertexAttribArray(aNormal);
        gl::BindVertexArray();

        return vao;
    }
};

osc::todo::GPU_mesh::GPU_mesh(Untextured_mesh const& um) :
    verts(reinterpret_cast<GLubyte const*>(um.verts.data()), um.verts.size()),
    indices(um.indices),
    instances{},
    main_vao(Gouraud_mrt_shader::create_vao<decltype(verts), Untextured_vert>(verts, indices, instances)),
    normal_vao(Normals_shader::create_vao<decltype(verts), Untextured_vert>(verts)),
    is_textured{false} {
}

osc::todo::GPU_mesh::GPU_mesh(Textured_mesh const& tm) :
    verts(reinterpret_cast<GLubyte const*>(tm.verts.data()), tm.verts.size()),
    indices(tm.indices),
    instances{},
    main_vao(Gouraud_mrt_shader::create_vao<decltype(verts), Textured_vert>(verts, indices, instances)),
    normal_vao(Normals_shader::create_vao<decltype(verts), Textured_vert>(verts)),
    is_textured{false} {
}

// Returns triangles of a "unit" (radius = 1.0f, origin = 0,0,0) sphere
static void unit_sphere_triangles(Untextured_mesh& out) {
    out.clear();

    // this is a shitty alg that produces a shitty UV sphere. I don't have
    // enough time to implement something better, like an isosphere, or
    // something like a patched sphere:
    //
    // https://www.iquilezles.org/www/articles/patchedsphere/patchedsphere.htm
    //
    // This one is adapted from:
    //    http://www.songho.ca/opengl/gl_sphere.html#example_cubesphere

    size_t sectors = 12;
    size_t stacks = 12;

    // polar coords, with [0, 0, -1] pointing towards the screen with polar
    // coords theta = 0, phi = 0. The coordinate [0, 1, 0] is theta = (any)
    // phi = PI/2. The coordinate [1, 0, 0] is theta = PI/2, phi = 0
    std::vector<Untextured_vert> points;

    float theta_step = 2.0f * pi_f / sectors;
    float phi_step = pi_f / stacks;

    for (size_t stack = 0; stack <= stacks; ++stack) {
        float phi = pi_f / 2.0f - static_cast<float>(stack) * phi_step;
        float y = sinf(phi);

        for (unsigned sector = 0; sector <= sectors; ++sector) {
            float theta = sector * theta_step;
            float x = sinf(theta) * cosf(phi);
            float z = -cosf(theta) * cosf(phi);
            glm::vec3 pos{x, y, z};
            glm::vec3 normal{pos};
            points.push_back({pos, normal});
        }
    }

    // the points are not triangles. They are *points of a triangle*, so the
    // points must be triangulated

    for (size_t stack = 0; stack < stacks; ++stack) {
        size_t k1 = stack * (sectors + 1);
        size_t k2 = k1 + sectors + 1;

        for (size_t sector = 0; sector < sectors; ++sector, ++k1, ++k2) {
            // 2 triangles per sector - excluding the first and last stacks
            // (which contain one triangle, at the poles)
            Untextured_vert p1 = points.at(k1);
            Untextured_vert p2 = points.at(k2);
            Untextured_vert p1_plus1 = points.at(k1 + 1u);
            Untextured_vert p2_plus1 = points.at(k2 + 1u);

            if (stack != 0) {
                out.verts.push_back(p1);
                out.verts.push_back(p1_plus1);
                out.verts.push_back(p2);
            }

            if (stack != (stacks - 1)) {
                out.verts.push_back(p1_plus1);
                out.verts.push_back(p2_plus1);
                out.verts.push_back(p2);
            }
        }
    }

    out.generate_trivial_indices();
}

static void simbody_cylinder_triangles(Untextured_mesh& out) {
    size_t num_sides = 12;

    out.clear();
    out.verts.reserve(2 * num_sides + 2 * num_sides);

    float step_angle = (2.0f * pi_f) / num_sides;
    float top_y = +1.0f;
    float bottom_y = -1.0f;

    // top
    {
        glm::vec3 normal = {0.0f, 1.0f, 0.0f};
        Untextured_vert top_middle{{0.0f, top_y, 0.0f}, normal};
        for (auto i = 0U; i < num_sides; ++i) {
            float theta_start = i * step_angle;
            float theta_end = (i + 1) * step_angle;

            // note: these are wound CCW for backface culling
            out.verts.push_back(top_middle);
            out.verts.push_back({glm::vec3(cosf(theta_end), top_y, sinf(theta_end)), normal});
            out.verts.push_back({glm::vec3(cosf(theta_start), top_y, sinf(theta_start)), normal});
        }
    }

    // bottom
    {
        glm::vec3 bottom_normal{0.0f, -1.0f, 0.0f};
        Untextured_vert top_middle{{0.0f, bottom_y, 0.0f}, bottom_normal};
        for (auto i = 0U; i < num_sides; ++i) {
            float theta_start = i * step_angle;
            float theta_end = (i + 1) * step_angle;

            // note: these are wound CCW for backface culling
            out.verts.push_back(top_middle);
            out.verts.push_back({
                glm::vec3(cosf(theta_start), bottom_y, sinf(theta_start)),
                bottom_normal,
            });
            out.verts.push_back({
                glm::vec3(cosf(theta_end), bottom_y, sinf(theta_end)),
                bottom_normal,
            });
        }
    }

    // sides
    {
        float norm_start = step_angle / 2.0f;
        for (auto i = 0U; i < num_sides; ++i) {
            float theta_start = i * step_angle;
            float theta_end = theta_start + step_angle;
            float norm_theta = theta_start + norm_start;

            glm::vec3 normal(cosf(norm_theta), 0.0f, sinf(norm_theta));
            glm::vec3 top1(cosf(theta_start), top_y, sinf(theta_start));
            glm::vec3 top2(cosf(theta_end), top_y, sinf(theta_end));

            glm::vec3 bottom1 = top1;
            bottom1.y = bottom_y;
            glm::vec3 bottom2 = top2;
            bottom2.y = bottom_y;

            // draw 2 triangles per quad cylinder side
            //
            // note: these are wound CCW for backface culling
            out.verts.push_back({top1, normal});
            out.verts.push_back({top2, normal});
            out.verts.push_back({bottom1, normal});

            out.verts.push_back({bottom2, normal});
            out.verts.push_back({bottom1, normal});
            out.verts.push_back({top2, normal});
        }
    }

    out.generate_trivial_indices();
}

// standard textured cube with dimensions [-1, +1] in xyz and uv coords of
// (0, 0) bottom-left, (1, 1) top-right for each (quad) face
static constexpr std::array<Textured_vert, 36> shaded_textured_cube_verts = {{
    // back face
    {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},  // bottom-left
    {{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},  // top-right
    {{1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},  // bottom-right
    {{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},  // top-right
    {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},  // bottom-left
    {{-1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},  // top-left
    // front face
    {{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},  // bottom-left
    {{1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},  // bottom-right
    {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},  // top-right
    {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},  // top-right
    {{-1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},  // top-left
    {{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},  // bottom-left
    // left face
    {{-1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},  // top-right
    {{-1.0f, 1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},  // top-left
    {{-1.0f, -1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},  // bottom-left
    {{-1.0f, -1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},  // bottom-left
    {{-1.0f, -1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},  // bottom-right
    {{-1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},  // top-right
    // right face
    {{1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},  // top-left
    {{1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},  // bottom-right
    {{1.0f, 1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},  // top-right
    {{1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},  // bottom-right
    {{1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},  // top-left
    {{1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},  // bottom-left
    // bottom face
    {{-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},  // top-right
    {{1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},  // top-left
    {{1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},  // bottom-left
    {{1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},  // bottom-left
    {{-1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},  // bottom-right
    {{-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},  // top-right
    // top face
    {{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},  // top-left
    {{1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},  // bottom-right
    {{1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},  // top-right
    {{1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},  // bottom-right
    {{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},  // top-left
    {{-1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}  // bottom-left
}};

// standard textured quad
// - dimensions [-1, +1] in xy and [0, 0] in z
// - uv coords are (0, 0) bottom-left, (1, 1) top-right
// - normal is +1 in Z, meaning that it faces toward the camera
static constexpr std::array<Textured_vert, 6> _shaded_textured_quad_verts = {{
    // CCW winding (culling)
    {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},  // bottom-left
    {{1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},  // bottom-right
    {{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},  // top-right

    {{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},  // top-right
    {{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},  // top-left
    {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},  // bottom-left
}};

static void simbody_brick_triangles(Untextured_mesh& out) {
    out.clear();

    for (Textured_vert v : shaded_textured_cube_verts) {
        out.verts.push_back({v.pos, v.normal});
    }

    out.generate_trivial_indices();
}

static void generate_floor_quad(Textured_mesh& out) {
    out.clear();

    for (Textured_vert v : _shaded_textured_quad_verts) {
        Textured_vert& tv = out.verts.emplace_back(v);
        tv.texcoord *= 200.0f;
    }

    out.generate_trivial_indices();
}

static void generate_NxN_grid(size_t n, Untextured_mesh& out) {
    static constexpr float z = 0.0f;
    static constexpr float min = -1.0f;
    static constexpr float max = 1.0f;

    size_t lines_per_dimension = n;
    float step_size = (max - min) / static_cast<float>(lines_per_dimension - 1);
    size_t num_lines = 2 * lines_per_dimension;
    size_t num_points = 2 * num_lines;

    out.clear();
    out.verts.resize(num_points);

    glm::vec3 normal = {0.0f, 0.0f, 0.0f};  // same for all

    size_t idx = 0;

    // lines parallel to X axis
    for (size_t i = 0; i < lines_per_dimension; ++i) {
        float y = min + i * step_size;

        Untextured_vert& p1 = out.verts[idx++];
        p1.pos = {-1.0f, y, z};
        p1.normal = normal;

        Untextured_vert& p2 = out.verts[idx++];
        p2.pos = {1.0f, y, z};
        p2.normal = normal;
    }

    // lines parallel to Y axis
    for (size_t i = 0; i < lines_per_dimension; ++i) {
        float x = min + i * step_size;

        Untextured_vert& p1 = out.verts[idx++];
        p1.pos = {x, -1.0f, z};
        p1.normal = normal;

        Untextured_vert& p2 = out.verts[idx++];
        p2.pos = {x, 1.0f, z};
        p2.normal = normal;
    }

    out.generate_trivial_indices();
}

static void generate_y_line(Untextured_mesh& out) {
    out.clear();
    out.verts.push_back({{0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}});
    out.verts.push_back({{0.0f, +1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}});
    out.generate_trivial_indices();
}

osc::todo::GPU_storage::GPU_storage() :
    // shaders
    shader_gouraud{new Gouraud_mrt_shader{}},
    shader_normals{new Normals_shader{}},
    shader_pts{new Plain_texture_shader{}},
    shader_cpts{new Colormapped_plain_texture_shader{}},
    shader_eds{new Edge_detection_shader{}},
    shader_skip_msxaa{new Skip_msxaa_blitter_shader{}} {

    // untextured preallocated meshes
    {
        Untextured_mesh utm;

        unit_sphere_triangles(utm);
        meshes.emplace_back(utm);
        simbody_sphere_idx = Mesh_idx::from_index(meshes.size() - 1);
        utm.clear();

        simbody_cylinder_triangles(utm);
        meshes.emplace_back(utm);
        simbody_cylinder_idx = Mesh_idx::from_index(meshes.size() - 1);
        utm.clear();

        simbody_brick_triangles(utm);
        meshes.emplace_back(utm);
        simbody_cube_idx = Mesh_idx::from_index(meshes.size() - 1);
        utm.clear();

        generate_NxN_grid(25, utm);
        meshes.emplace_back(utm);
        grid_25x25_idx = Mesh_idx::from_index(meshes.size() - 1);
        utm.clear();

        generate_y_line(utm);
        meshes.emplace_back(utm);
        yline_idx = Mesh_idx::from_index(meshes.size() - 1);
        utm.clear();
    }

    // textured preallocated meshes
    {
        Textured_mesh tm;

        generate_floor_quad(tm);
        meshes.emplace_back(tm);
        floor_quad_idx = Mesh_idx::from_index(meshes.size() - 1);
        tm.clear();

        for (Textured_vert const& tv : _shaded_textured_quad_verts) {
            tm.verts.push_back(tv);
        }
        tm.generate_trivial_indices();
        meshes.emplace_back(tm);
        quad_idx = Mesh_idx::from_index(meshes.size() - 1);
        quad_vbo = gl::Array_buffer<Textured_vert>(tm.verts);
    }

    // preallocated textures
    textures.push_back(generate_chequered_floor_texture());
    chequer_idx = Tex_idx::from_index(textures.size() - 1);

    eds_quad_vao = Edge_detection_shader::create_vao(quad_vbo);
    skip_msxaa_quad_vao = Skip_msxaa_blitter_shader::create_vao(quad_vbo);
    pts_quad_vao = Plain_texture_shader::create_vao(quad_vbo);
    cpts_quad_vao = Colormapped_plain_texture_shader::create_vao(quad_vbo);
}

osc::todo::GPU_storage::GPU_storage(GPU_storage&&) noexcept = default;

osc::todo::GPU_storage& osc::todo::GPU_storage::operator=(GPU_storage&&) noexcept = default;

osc::todo::GPU_storage::~GPU_storage() noexcept = default;

osc::todo::Render_target::Render_target(int w_, int h_, int samples_) :
    w{w_},
    h{h_},
    samples{samples_},

    scene_rgba{[this]() {
        gl::Render_buffer rv;
        gl::BindRenderBuffer(rv);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA, w, h);
        return rv;
    }()},

    scene_passthrough{[this]() {
        gl::Texture_2d_multisample rv;
        gl::BindTexture(rv);
        glTexImage2DMultisample(rv.type, samples, GL_RGB, w, h, GL_TRUE);
        return rv;
    }()},

    scene_depth24stencil8{[this]() {
        gl::Render_buffer rv;
        gl::BindRenderBuffer(rv);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, w, h);
        return rv;
    }()},

    scene_fbo{[this]() {
        gl::Frame_buffer rv;
        gl::BindFramebuffer(GL_FRAMEBUFFER, rv);
        gl::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, scene_rgba);
        gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, scene_passthrough, 0);
        gl::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, scene_depth24stencil8);
        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::window_fbo);
        return rv;
    }()},

    passthrough_nomsxaa{[this]() {
        gl::Texture_2d rv;
        gl::BindTexture(rv);
        gl::TexImage2D(rv.type, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        return rv;
    }()},

    passthrough_fbo{[this]() {
        gl::Frame_buffer rv;
        gl::BindFramebuffer(GL_FRAMEBUFFER, rv);
        gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, passthrough_nomsxaa, 0);
        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::window_fbo);
        return rv;
    }()},

    passthrough_pbos{{
        {0x00, 0x00, 0x00, 0x00},  // rgba
        {0x00, 0x00, 0x00, 0x00},  // rgba
    }},

    passthrough_pbo_cur{0},  // 0/1

    scene_tex_resolved{[this]() {
        gl::Texture_2d rv;
        gl::BindTexture(rv);
        gl::TexImage2D(rv.type, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        gl::TexParameteri(rv.type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // no mipmaps
        gl::TexParameteri(rv.type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // no mipmaps
        return rv;
    }()},

    scene_fbo_resolved{[this]() {
        gl::Frame_buffer rv;
        gl::BindFramebuffer(GL_FRAMEBUFFER, rv);
        gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, scene_tex_resolved, 0);
        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::window_fbo);
        return rv;
    }()},

    passthrough_tex_resolved{[this]() {
        gl::Texture_2d rv;
        gl::BindTexture(rv);
        gl::TexImage2D(rv.type, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        gl::TexParameteri(rv.type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // no mipmaps
        gl::TexParameteri(rv.type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // no mipmaps
        return rv;
    }()},

    passthrough_fbo_resolved{[this]() {
        gl::Frame_buffer rv;
        gl::BindFramebuffer(GL_FRAMEBUFFER, rv);
        gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, passthrough_tex_resolved, 0);
        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::window_fbo);
        return rv;
    }()},

    hittest_result{} {

    hittest_result.b0 = 0x00;
    hittest_result.b1 = 0x00;
}

void osc::todo::Render_target::reconfigure(int w_, int h_, int samples_) {
    if (w != w_ || h != h_ || samples != samples_) {
        *this = Render_target{w_, h_, samples_};
    }
}

void osc::todo::draw_scene(GPU_storage& storage, Render_params const& params, Drawlist const& drawlist, Render_target& out) {

    // overview:
    //
    // drawing the scene efficiently is a fairly involved process. I apologize for that, but
    // rendering scenes efficiently with OpenGL requires has to keep OpenGL, GPUs, and API
    // customization in-mind - while also playing ball with the OpenSim API.
    //
    // this is a forward (as opposed to deferred) renderer that borrows some ideas from deferred
    // rendering techniques. It *mostly* draws the entire scene in one pass (forward rending) but
    // the rendering step *also* writes to a multi-render-target (MRT) FBO that extra information
    // such as what's currently selected, and it uses that information in downstream sampling steps
    // (kind of like how deferred rendering puts everything into information-dense buffers). The
    // reason this rendering pipeline isn't fully deferred (gbuffers, albeido, etc.) is because
    // the scene is lit by a single directional light and the shading is fairly simple.

    Mesh_instance const* meshes = drawlist._instances.data();
    size_t nmeshes = drawlist._instances.size();

    glViewport(0, 0, out.w, out.h);

    // bind to an off-screen framebuffer object (FBO)
    //
    // drawing into this FBO writes to textures that the user can't see, but that can
    // be sampled by downstream shaders
    gl::BindFramebuffer(GL_FRAMEBUFFER, out.scene_fbo);

    // clear the scene FBO's draw buffers for a new draw call
    //
    //   - COLOR0: main scene render: fill in background
    //   - COLOR1: RGB passthrough (selection logic + rim alpa): blank out all channels
    gl::DrawBuffer(GL_COLOR_ATTACHMENT0);
    gl::ClearColor(params.background_rgba);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gl::DrawBuffer(GL_COLOR_ATTACHMENT1);
    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT);

    // handle wireframe mode: should only be enabled for scene + floor render: the other
    // renders will render to a screen-sized quad
    GLenum original_poly_mode = gl::GetEnum(GL_POLYGON_MODE);
    if (params.flags & DrawcallFlags_WireframeMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // render the scene to the FBO using a multiple-render-target (MRT) multisampled
    // (MSXAAed) shader.
    //
    // FBO outputs are:
    //
    // - COLOR0: main target: multisampled scene geometry
    //     - the input color is Gouraud-shaded based on light parameters etc.
    // - COLOR1: RGB passthrough: written to output as-is
    //     - the input color encodes the selected component index (RG) and the rim
    //       alpha (B). It's used in downstream steps
    if (params.flags & RawRendererFlags_DrawSceneGeometry) {
        Gouraud_mrt_shader& shader = *storage.shader_gouraud;

        gl::DrawBuffers(GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1);
        gl::UseProgram(shader.program);
        gl::Uniform(shader.uProjMat, params.projection_matrix);
        gl::Uniform(shader.uViewMat, params.view_matrix);
        gl::Uniform(shader.uLightPos, params.light_pos);
        gl::Uniform(shader.uLightColor, params.light_rgb);
        gl::Uniform(shader.uViewPos, params.view_pos);

        // blending:
        //     COLOR0 should be blended because OpenSim scenes can contain blending
        //     COLOR1 should never be blended: it's a value for the top-most fragment
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnablei(GL_BLEND, 0);
        glDisablei(GL_BLEND, 1);

        if (params.flags & RawRendererFlags_UseInstancedRenderer) {
            size_t pos = 0;
            while (pos < nmeshes) {
                Mesh_idx meshidx = meshes[pos].meshidx;
                Tex_idx texidx = meshes[pos].texidx;
                Instance_flags flags = meshes[pos].flags;
                size_t end = pos + 1;

                while (end < nmeshes && meshes[end].meshidx == meshidx && meshes[end].texidx == texidx &&
                       meshes[end].flags == flags) {

                    ++end;
                }

                // [pos, end) contains instances with the same meshid + textureid + flags

                // texture-related stuff
                if (texidx.is_valid()) {
                    gl::Uniform(shader.uIsTextured, true);
                    gl::ActiveTexture(GL_TEXTURE0);
                    gl::BindTexture(storage.textures[texidx.to_index()]);
                    gl::Uniform(shader.uSampler0, gl::texture_index<GL_TEXTURE0>());
                } else {
                    gl::Uniform(shader.uIsTextured, false);
                }

                // flag-related stuff
                gl::Uniform(shader.uIsShaded, flags.is_shaded());
                gl::Uniform(shader.uSkipVP, flags.skip_view_projection());
                GLenum mode = flags.mode();

                GPU_mesh& gm = storage.meshes[meshidx.to_index()];
                gm.instances.assign(meshes + pos, end - pos);
                gl::BindVertexArray(gm.main_vao);

                glDrawElementsInstanced(
                    mode, gm.indices.sizei(), gl::index_type(gm.indices), nullptr, static_cast<GLsizei>(end - pos));
                gl::BindVertexArray();

                pos = end;
            }
        } else {
            // perform (slower) one-drawcall-per-item rendering
            //
            // this is here mostly for perf comparison and debugging

            for (size_t i = 0; i < nmeshes; ++i) {
                Mesh_instance const& mi = meshes[i];

                // texture-related stuff
                if (mi.texidx.is_valid()) {
                    gl::Uniform(shader.uIsTextured, true);
                    gl::ActiveTexture(GL_TEXTURE0);
                    gl::BindTexture(storage.textures[mi.texidx.to_index()]);
                    gl::Uniform(shader.uSampler0, gl::texture_index<GL_TEXTURE0>());
                } else {
                    gl::Uniform(shader.uIsTextured, false);
                }

                // flag-related stuff
                gl::Uniform(shader.uIsShaded, mi.flags.is_shaded());
                gl::Uniform(shader.uSkipVP, mi.flags.skip_view_projection());
                GLenum mode = mi.flags.mode();

                GPU_mesh& gm = storage.meshes[mi.meshidx.to_index()];
                gm.instances.assign(meshes + i, 1);
                gl::BindVertexArray(gm.main_vao);
                glDrawElementsInstanced(mode, gm.indices.sizei(), gl::index_type(gm.indices), nullptr, static_cast<GLsizei>(1));
                gl::BindVertexArray();
            }
        }

        glDisablei(GL_BLEND, 0);
    }

    glPolygonMode(GL_FRONT_AND_BACK, original_poly_mode);

    // (optional): render scene normals into COLOR0
    if (params.flags & DrawcallFlags_ShowMeshNormals) {
        Normals_shader& shader = *storage.shader_normals;
        gl::DrawBuffer(GL_COLOR_ATTACHMENT0);
        gl::UseProgram(shader.program);
        gl::Uniform(shader.uProjMat, params.projection_matrix);
        gl::Uniform(shader.uViewMat, params.view_matrix);

        for (size_t i = 0; i < nmeshes; ++i) {
            Mesh_instance const& mi = meshes[i];
            GPU_mesh& gm = storage.meshes[mi.meshidx.to_index()];
            gl::Uniform(shader.uModelMat, mi.model_xform);
            gl::Uniform(shader.uNormalMat, mi.normal_xform);
            gl::BindVertexArray(gm.normal_vao);
            gl::DrawArrays(GL_TRIANGLES, 0, gm.verts.sizei() / (gm.is_textured ? sizeof(Textured_vert) : sizeof(Untextured_vert)));
        }
        gl::BindVertexArray();
    }

    // perform passthrough hit testing
    //
    // in the previous draw call, COLOR1's RGB channels encoded arbitrary passthrough data
    // Extracting that pixel value (without MSXAA blending) and decoding it yields the
    // user-supplied data
    //
    // this makes it possible for renderer users (e.g. OpenSim model renderer) to encode
    // model information (e.g. "a component index") into screenspace

    Passthrough_data hittest_result{};
    if (params.flags & RawRendererFlags_PerformPassthroughHitTest) {
        // (temporarily) set the OpenGL viewport to a small square around the hit testing
        // location
        //
        // this causes the subsequent draw call to only run the fragment shader around where
        // we actually care about
        glViewport(params.passthrough_hittest_x - 1, params.passthrough_hittest_y - 1, 3, 3);

        // bind to a non-MSXAAed FBO
        gl::BindFramebuffer(GL_FRAMEBUFFER, out.passthrough_fbo);
        gl::DrawBuffer(GL_COLOR_ATTACHMENT0);

        // use a specialized shader that is MSXAA-aware to blit exactly one non-blended AA
        // sample from COLOR1 to the output
        //
        // by deliberately avoiding MSXAA, every value in this output should be exactly the
        // same as the passthrough value provided by the caller
        Skip_msxaa_blitter_shader& shader = *storage.shader_skip_msxaa;
        gl::UseProgram(shader.p);
        gl::Uniform(shader.uModelMat, gl::identity_val);
        gl::Uniform(shader.uViewMat, gl::identity_val);
        gl::Uniform(shader.uProjMat, gl::identity_val);
        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(out.scene_passthrough);
        gl::Uniform(shader.uSampler0, gl::texture_index<GL_TEXTURE0>());
        gl::BindVertexArray(storage.skip_msxaa_quad_vao);
        gl::DrawArrays(GL_TRIANGLES, 0, storage.quad_vbo.sizei());
        gl::BindVertexArray();

        // reset viewport
        glViewport(0, 0, out.w, out.h);

        // the FBO now contains non-MSXAAed version of COLOR1

        // read the pixel under the mouse
        //
        // - you *could* just read the value directly from the FBO with `glReadPixels`, which is
        //   what the first iteration of this alg. did (non optimized)
        //
        // - However, that glReadPixels call will screw performance. On my machine (Ryzen1600 /w
        //   Geforce 1060), it costs around 30 % FPS (300 FPS --> 200 FPS)
        //
        // - This isn't because the transfer is expensive--it's just a single pixel, after all--but
        //   because reading the pixel forces the OpenGL driver to flush all pending rendering
        //   operations to the FBO (a "pipeline stall")
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

        if (params.flags & RawRendererFlags_UseOptimizedButDelayed1FrameHitTest) {
            size_t reader = out.passthrough_pbo_cur % out.passthrough_pbos.size();
            size_t mapper = (out.passthrough_pbo_cur + 1) % out.passthrough_pbos.size();

            // launch asynchronous request for this frame's pixel
            gl::BindBuffer(out.passthrough_pbos[reader]);
            glReadPixels(
                params.passthrough_hittest_x, params.passthrough_hittest_y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

            // synchrnously read *last frame's* pixel
            gl::BindBuffer(out.passthrough_pbos[mapper]);
            GLubyte* src = static_cast<GLubyte*>(glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY));

            // note: these values are the *last frame*'s
            hittest_result.b0 = src[0];
            hittest_result.b1 = src[1];

            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);

            // flip PBOs ready for next frame
            out.passthrough_pbo_cur = (out.passthrough_pbo_cur+ 1) % out.passthrough_pbos.size();
        } else {
            // slow mode: synchronously read the current frame's pixel under the cursor
            //
            // this is kept here so that people can try it out if selection logic is acting
            // bizzarely (e.g. because it is delayed one frame)

            glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
            GLubyte rgba[3]{};
            glReadPixels(
                params.passthrough_hittest_x, params.passthrough_hittest_y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, rgba);

            hittest_result.b0 = rgba[0];
            hittest_result.b1 = rgba[1];
        }
    }

    // resolve MSXAA in COLOR0 to output texture
    //
    // "resolve" (i.e. blend) the MSXAA samples in COLOR0. We are "done" with
    // COLOR0. You might expect we can directly blit it to the output, but that
    // seems to explode with some OpenGL drivers (e.g. Intel iGPUs like UHD 620)
    {
        gl::BindFramebuffer(GL_READ_FRAMEBUFFER, out.scene_fbo);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        gl::BindFramebuffer(GL_DRAW_FRAMEBUFFER, out.scene_fbo_resolved);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        gl::BlitFramebuffer(0, 0, out.w, out.h, 0, 0, out.w, out.h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    // resolve MSXAA in COLOR1
    //
    // "resolve" (i.e. blend) the MSXAA samples in COLOR1 into non-MSXAAed textures
    // that the edge-detection shader can sample normally
    {
        gl::BindFramebuffer(GL_READ_FRAMEBUFFER, out.scene_fbo);
        glReadBuffer(GL_COLOR_ATTACHMENT1);
        gl::BindFramebuffer(GL_DRAW_FRAMEBUFFER, out.passthrough_fbo_resolved);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        gl::BlitFramebuffer(0, 0, out.w, out.h, 0, 0, out.w, out.h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    // bind to output texture: all further drawing goes onto it
    gl::BindFramebuffer(GL_FRAMEBUFFER, out.scene_fbo_resolved);

    // draw rims highlights onto the output
    //
    // COLOR1's alpha channel contains *filled in shapes* for each element in the scene that
    // should be rim-shaded. Those shapes are exactly the same as the scene geometry, so showing
    // them as-is would be pointless (they'd either entirely occlude, or be occluded by, the scene)
    //
    // rim-highlighting puts a rim around the outer edge of the geometry. There are various tricks
    // for doing this, such as rendering the geometry twice - the second time backface-enlarged
    // slightly, or holding onto two versions of every mesh (normal mesh, normal-scaled mesh),
    // but those techniques each have drawbacks (e.g. more draw calls, fails with non-convex
    // geometry, behaves bizzarely with non-centered meshes)
    //
    // this technique performs rim highlighting in screen-space using a standard edge-detection
    // kernel. The drawback of this is that every single pixel in the screen has to be
    // edge-detected, and the rims are in screen-space, rather than world space (so they don't
    // "zoom out" as if they were "in the scene"). However, GPUs are fairly efficient at running
    // branchless kernel lookups over a screen, so it isn't as expensive as you think
    if (params.flags & DrawcallFlags_DrawRims) {
        Edge_detection_shader& shader = *storage.shader_eds;
        gl::UseProgram(shader.p);
        gl::Uniform(shader.uModelMat, gl::identity_val);
        gl::Uniform(shader.uViewMat, gl::identity_val);
        gl::Uniform(shader.uProjMat, gl::identity_val);
        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(out.passthrough_tex_resolved);
        gl::Uniform(shader.uSampler0, gl::texture_index<GL_TEXTURE0>());
        gl::Uniform(shader.uRimRgba, params.rim_rgba);

        float rim_thickness = 2.0f / std::max(out.w, out.h);

        gl::Uniform(shader.uRimThickness, rim_thickness);

        glEnable(GL_BLEND);  // rims can have alpha
        glDisable(GL_DEPTH_TEST);
        gl::BindVertexArray(storage.eds_quad_vao);
        gl::DrawArrays(GL_TRIANGLES, 0, storage.quad_vbo.sizei());
        gl::BindVertexArray();
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    }

    // render debug quads onto output (if applicable)
    if (params.flags & RawRendererFlags_DrawDebugQuads) {
        Colormapped_plain_texture_shader& cpts = *storage.shader_cpts;
        gl::UseProgram(cpts.p);
        gl::BindVertexArray(storage.pts_quad_vao);

        // COLOR1 quad (RGB)
        if (true) {
            glm::mat4 row1 = []() {
                glm::mat4 m = glm::identity<glm::mat4>();
                m = glm::translate(m, glm::vec3{0.80f, 0.80f, -1.0f});  // move to [+0.6, +1.0] in x
                m = glm::scale(m, glm::vec3{0.20f});  // so it becomes [-0.2, +0.2]
                return m;
            }();

            gl::Uniform(cpts.uMVP, row1);
            gl::ActiveTexture(GL_TEXTURE0);
            gl::BindTexture(out.passthrough_tex_resolved);
            gl::Uniform(cpts.uSampler0, gl::texture_index<GL_TEXTURE0>());
            gl::Uniform(cpts.uSamplerMultiplier, gl::identity_val);
            gl::DrawArrays(GL_TRIANGLES, 0, storage.quad_vbo.sizei());
        }

        // COLOR1 quad (A)
        if (true) {
            glm::mat4 row2 = []() {
                glm::mat4 m = glm::identity<glm::mat4>();
                m = glm::translate(m, glm::vec3{0.80f, 0.40f, -1.0f});  // move to [+0.6, +1.0] in x
                m = glm::scale(m, glm::vec3{0.20f});  // so it becomes [-0.2, +0.2]
                return m;
            }();

            glm::mat4 alpha2rgb = {
                // column-major
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                1.0f,
                1.0f,
                1.0f,
                1.0f,
            };

            gl::Uniform(cpts.uMVP, row2);
            gl::ActiveTexture(GL_TEXTURE0);
            gl::BindTexture(out.passthrough_tex_resolved);
            gl::Uniform(cpts.uSampler0, gl::texture_index<GL_TEXTURE0>());
            gl::Uniform(cpts.uSamplerMultiplier, alpha2rgb);
            gl::DrawArrays(GL_TRIANGLES, 0, storage.quad_vbo.sizei());
        }

        gl::BindVertexArray();
    }

    // bind back to the original framebuffer (assumed to be window)
    gl::BindFramebuffer(GL_FRAMEBUFFER, gl::window_fbo);

    out.hittest_result = hittest_result;
}

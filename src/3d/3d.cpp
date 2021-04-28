#include "src/3d/3d.hpp"

#include "src/config.hpp"
#include "src/utils/helpers.hpp"
#include "src/utils/sfinae.hpp"
#include "src/constants.hpp"
#include "src/3d/texturing.hpp"

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

osc::todo::Render_target::Render_target(int w, int h, int samples) {
    // TODO
}

void osc::todo::Render_target::reconfigure(int w, int h, int samples) {
    // TODO
}

void osc::todo::draw_scene(GPU_storage const&, Render_params const&, Drawlist const&, Render_target&) {
    // TODO
}

#include "mesh_instance_meshdata_screen.hpp"

#include "src/app.hpp"
#include "src/log.hpp"
#include "src/3d/3d.hpp"
#include "src/3d/instanced_renderer.hpp"

using namespace osc;

static char const* const g_VertexShader = R"(
    #version 330 core

    uniform mat4 uModel;
    uniform mat4 uView;
    uniform mat4 uProjection;

    layout (location = 0) in vec3 aPos;

    void main() {
        gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
    }
)";

static char const* const g_FragmentShader = R"(
    #version 330 core

    uniform vec4 uColor;

    out vec4 FragColor;

    void main() {
        FragColor = uColor;
    }
)";

namespace {
    struct Basic_shader final {
        gl::Program program = gl::CreateProgramFrom(
            gl::CompileFromSource<gl::Vertex_shader>(g_VertexShader),
            gl::CompileFromSource<gl::Fragment_shader>(g_FragmentShader));

        gl::Attribute_vec3 aPos{0};
        gl::Uniform_mat4 uModel = gl::GetUniformLocation(program, "uModel");
        gl::Uniform_mat4 uView = gl::GetUniformLocation(program, "uView");
        gl::Uniform_mat4 uProjection = gl::GetUniformLocation(program, "uProjection");
        gl::Uniform_vec4 uColor = gl::GetUniformLocation(program, "uColor");
    };
}

static gl::Vertex_array make_vao(Basic_shader& shader, Mesh_instance_meshdata& md) {
    gl::Vertex_array rv;
    gl::BindVertexArray(rv);
    gl::BindBuffer(md.verts);
    gl::VertexAttribPointer(shader.aPos, false, sizeof(Untextured_vert), offsetof(Untextured_vert, pos));
    gl::EnableVertexAttribArray(shader.aPos);
    gl::BindBuffer(md.indices);
    gl::BindVertexArray();
    return rv;
}

struct osc::Mesh_instance_meshdata_screen::Impl final {
    Basic_shader shader;
    Untextured_mesh mesh = generate_cube<Untextured_mesh>();
    Mesh_instance_meshdata gpu_mesh{mesh};
    gl::Vertex_array vao = make_vao(shader, gpu_mesh);
};

osc::Mesh_instance_meshdata_screen::Mesh_instance_meshdata_screen() :
    m_Impl{new Impl{}} {
}

osc::Mesh_instance_meshdata_screen::~Mesh_instance_meshdata_screen() noexcept = default;

void osc::Mesh_instance_meshdata_screen::draw() {
    App::cur().enable_debug_mode();
    Impl& impl = *m_Impl;
    Basic_shader& shader = impl.shader;

    glm::mat4 model_mtx = glm::scale(glm::mat4{1.0f}, {0.1f, 0.1f, 0.1f});

    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gl::UseProgram(shader.program);
    gl::Uniform(shader.uModel, model_mtx);
    gl::Uniform(shader.uView, gl::identity_val);
    gl::Uniform(shader.uProjection, gl::identity_val);
    gl::Uniform(shader.uColor, {1.0f, 0.0f, 0.0f, 1.0f});
    gl::BindVertexArray(impl.vao);
    gl::DrawElements(GL_TRIANGLES, impl.gpu_mesh.indices.sizei(), gl::index_type(impl.gpu_mesh.indices), nullptr);
    gl::BindVertexArray();
}

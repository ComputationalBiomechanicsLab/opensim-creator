#include "tut1_hellotriangle_screen.hpp"

#include "src/app.hpp"
#include "src/3d/gl.hpp"
#include "src/3d/gl_glm.hpp"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

using namespace osc;

static constexpr char const g_VertexShader[] = R"(
    #version 330 core

    in vec3 aPos;

    void main() {
        gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
    }
)";

static constexpr char const g_FragmentShader[] = R"(
    #version 330 core

    out vec4 FragColor;
    uniform vec4 uColor;

    void main() {
        FragColor = uColor;
    }
)";

namespace {
    struct Shader final {
        gl::Program program = gl::CreateProgramFrom(
            gl::CompileFromSource<gl::Vertex_shader>(g_VertexShader),
            gl::CompileFromSource<gl::Fragment_shader>(g_FragmentShader));

        gl::Attribute_vec3 aPos = gl::GetAttribLocation(program, "aPos");
        gl::Uniform_vec4 uColor = gl::GetUniformLocation(program, "uColor");
    };

    gl::Vertex_array create_vao(Shader& shader, gl::Array_buffer<glm::vec3> const& points) {
        gl::Vertex_array rv;

        gl::BindVertexArray(rv);
        gl::BindBuffer(points);
        gl::VertexAttribPointer(shader.aPos, false, sizeof(glm::vec3), 0);
        gl::EnableVertexAttribArray(shader.aPos);
        gl::BindVertexArray();

        return rv;
    }
}

struct osc::Tut1_hellotriangle_screen::Impl final {
    Shader shader;

    gl::Array_buffer<glm::vec3> points = {
        {-1.0f, -1.0f, 0.0f},
        {+0.0f, +1.0f, 0.0f},
        {+1.0f, -1.0f, 0.0f}
    };

    gl::Vertex_array vao = create_vao(shader, points);

    float fade_speed = 1.0f;
    glm::vec4 color = {1.0f, 0.0f, 0.0f, 1.0f};
};

// public API

osc::Tut1_hellotriangle_screen::Tut1_hellotriangle_screen() :
    impl{new Impl{}} {
}

osc::Tut1_hellotriangle_screen::~Tut1_hellotriangle_screen() noexcept = default;

void osc::Tut1_hellotriangle_screen::tick(float dt) {
    // change color over time

    if (impl->color.r < 0.0f || impl->color.r > 1.0f) {
        impl->fade_speed = -impl->fade_speed;
    }

    impl->color.r -= dt * impl->fade_speed;
}

void osc::Tut1_hellotriangle_screen::draw() {
    gl::Viewport(0, 0, App::cur().idims().x, App::cur().idims().y);
    gl::ClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gl::UseProgram(impl->shader.program);
    gl::Uniform(impl->shader.uColor, impl->color);
    gl::BindVertexArray(impl->vao);
    gl::DrawArrays(GL_TRIANGLES, 0, impl->points.sizei());
    gl::BindVertexArray();
}

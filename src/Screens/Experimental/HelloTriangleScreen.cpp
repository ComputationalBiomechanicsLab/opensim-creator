#include "HelloTriangleScreen.hpp"

#include "src/App.hpp"
#include "src/3D/Gl.hpp"
#include "src/3D/GlGlm.hpp"
#include "src/Screens/Experimental/ExperimentsScreen.hpp"

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
    struct BasicShader final {
        gl::Program program = gl::CreateProgramFrom(
            gl::CompileFromSource<gl::VertexShader>(g_VertexShader),
            gl::CompileFromSource<gl::FragmentShader>(g_FragmentShader));

        gl::AttributeVec3 aPos = gl::GetAttribLocation(program, "aPos");
        gl::UniformVec4 uColor = gl::GetUniformLocation(program, "uColor");
    };
}

static gl::VertexArray createVAO(BasicShader& shader, gl::ArrayBuffer<glm::vec3> const& points) {
    gl::VertexArray rv;

    gl::BindVertexArray(rv);
    gl::BindBuffer(points);
    gl::VertexAttribPointer(shader.aPos, false, sizeof(glm::vec3), 0);
    gl::EnableVertexAttribArray(shader.aPos);
    gl::BindVertexArray();

    return rv;
}

struct osc::HelloTriangleScreen::Impl final {
    BasicShader shader;

    gl::ArrayBuffer<glm::vec3> points = {
        {-1.0f, -1.0f, 0.0f},
        {+1.0f, -1.0f, 0.0f},
        {+0.0f, +1.0f, 0.0f},
    };

    gl::VertexArray vao = createVAO(shader, points);

    float fadeSpeed = 1.0f;
    glm::vec4 color = {1.0f, 0.0f, 0.0f, 1.0f};
};

// public API

osc::HelloTriangleScreen::HelloTriangleScreen() :
    m_Impl{new Impl{}} {
}

osc::HelloTriangleScreen::~HelloTriangleScreen() noexcept = default;

void osc::HelloTriangleScreen::onEvent(SDL_Event const& e) {
    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        App::cur().requestTransition<ExperimentsScreen>();
    }
}

void osc::HelloTriangleScreen::tick(float dt) {

    if (m_Impl->color.r < 0.0f || m_Impl->color.r > 1.0f) {
        m_Impl->fadeSpeed = -m_Impl->fadeSpeed;
    }

    m_Impl->color.r -= dt * m_Impl->fadeSpeed;
}

void osc::HelloTriangleScreen::draw() {
    gl::Viewport(0, 0, App::cur().idims().x, App::cur().idims().y);
    gl::ClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gl::UseProgram(m_Impl->shader.program);
    gl::Uniform(m_Impl->shader.uColor, m_Impl->color);
    gl::BindVertexArray(m_Impl->vao);
    gl::DrawArrays(GL_TRIANGLES, 0, m_Impl->points.sizei());
    gl::BindVertexArray();
}

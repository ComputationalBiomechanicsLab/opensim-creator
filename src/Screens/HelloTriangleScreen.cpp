#include "HelloTriangleScreen.hpp"

#include "src/Graphics/Gl.hpp"
#include "src/Graphics/GlGlm.hpp"
#include "src/Platform/App.hpp"
#include "src/Screens/ExperimentsScreen.hpp"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

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

namespace
{
    struct BasicShader final {
        gl::Program program = gl::CreateProgramFrom(
            gl::CompileFromSource<gl::VertexShader>(g_VertexShader),
            gl::CompileFromSource<gl::FragmentShader>(g_FragmentShader));

        gl::AttributeVec3 aPos = gl::GetAttribLocation(program, "aPos");
        gl::UniformVec4 uColor = gl::GetUniformLocation(program, "uColor");
    };
}

static gl::VertexArray createVAO(BasicShader& shader, gl::ArrayBuffer<glm::vec3> const& points)
{
    gl::VertexArray rv;

    gl::BindVertexArray(rv);
    gl::BindBuffer(points);
    gl::VertexAttribPointer(shader.aPos, false, sizeof(glm::vec3), 0);
    gl::EnableVertexAttribArray(shader.aPos);
    gl::BindVertexArray();

    return rv;
}

class osc::HelloTriangleScreen::Impl final {
public:

    void onEvent(SDL_Event const& e)
    {
        if (e.type == SDL_QUIT)
        {
            App::upd().requestQuit();
            return;
        }
        else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
        {
            App::upd().requestTransition<ExperimentsScreen>();
            return;
        }
    }

    void tick()
    {
        if (m_Color.r < 0.0f || m_Color.r > 1.0f)
        {
            m_FadeSpeed = -m_FadeSpeed;
        }

        m_Color.r -= osc::App::get().getDeltaSinceLastFrame().count() * m_FadeSpeed;
    }

    void draw()
    {
        gl::Viewport(0, 0, App::get().idims().x, App::get().idims().y);
        gl::ClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        gl::UseProgram(m_Shader.program);
        gl::Uniform(m_Shader.uColor, m_Color);
        gl::BindVertexArray(m_VAO);
        gl::DrawArrays(GL_TRIANGLES, 0, m_Points.sizei());
        gl::BindVertexArray();
    }

private:
    BasicShader m_Shader;

    gl::ArrayBuffer<glm::vec3> m_Points = {
        {-1.0f, -1.0f, 0.0f},
        {+1.0f, -1.0f, 0.0f},
        {+0.0f, +1.0f, 0.0f},
    };

    gl::VertexArray m_VAO = createVAO(m_Shader, m_Points);

    float m_FadeSpeed = 1.0f;
    glm::vec4 m_Color = {1.0f, 0.0f, 0.0f, 1.0f};
};


// public API (PIMPL)

osc::HelloTriangleScreen::HelloTriangleScreen() :
    m_Impl{new Impl{}}
{
}

osc::HelloTriangleScreen::HelloTriangleScreen(HelloTriangleScreen&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::HelloTriangleScreen& osc::HelloTriangleScreen::operator=(HelloTriangleScreen&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::HelloTriangleScreen::~HelloTriangleScreen() noexcept
{
    delete m_Impl;
}

void osc::HelloTriangleScreen::onEvent(SDL_Event const& e)
{
    m_Impl->onEvent(e);
}

void osc::HelloTriangleScreen::tick()
{
    m_Impl->tick();
}

void osc::HelloTriangleScreen::draw()
{
    m_Impl->draw();
}

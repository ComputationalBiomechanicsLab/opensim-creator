#include "RendererHelloTriangleScreen.hpp"

#include "src/3D/Renderer.hpp"
#include "src/App.hpp"

#include <utility>
#include <imgui.h>

static constexpr char const g_Shader[] = R"(
    BEGIN_VERTEX_SHADER

    #version 330 core

    in vec3 aPos;

    void main() {
        gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
    }

    END_VERTEX_SHADER

    BEGIN_FRAGMENT_SHADER

    #version 330 core

    out vec4 FragColor;
    uniform vec4 uColor;

    void main() {
        FragColor = uColor;
    }

    END_FRAGMENT_SHADER
)";


class osc::RendererHelloTriangleScreen::Impl final {
public:
    void onMount()
    {
        ImGuiInit();
    }

    void onUnmount()
    {
        ImGuiShutdown();
    }

    void onEvent(SDL_Event const& e)
    {
        ImGuiOnEvent(e);
    }

    void tick(float)
    {
    }

    void draw()
    {
        App::cur().clearScreen({0.0f, 0.0f, 0.0f, 0.0f});

        ImGuiNewFrame();
        if (ImGui::Begin("panel"))
        {
            ImGui::Text("hi");
        }
        ImGui::End();
        ImGuiRender();
    }

private:
    experimental::Shader m_Shader{g_Shader};
};

osc::RendererHelloTriangleScreen::RendererHelloTriangleScreen() :
    m_Impl{new Impl{}}
{
}

osc::RendererHelloTriangleScreen::RendererHelloTriangleScreen(RendererHelloTriangleScreen&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::RendererHelloTriangleScreen& osc::RendererHelloTriangleScreen::operator=(RendererHelloTriangleScreen&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::RendererHelloTriangleScreen::~RendererHelloTriangleScreen() noexcept
{
    delete m_Impl;
}

void osc::RendererHelloTriangleScreen::onMount()
{
    m_Impl->onMount();
}

void osc::RendererHelloTriangleScreen::onUnmount()
{
    m_Impl->onUnmount();
}

void osc::RendererHelloTriangleScreen::onEvent(SDL_Event const& e)
{
    m_Impl->onEvent(e);
}

void osc::RendererHelloTriangleScreen::tick(float dt)
{
    m_Impl->tick(dt);
}

void osc::RendererHelloTriangleScreen::draw()
{
    m_Impl->draw();
}

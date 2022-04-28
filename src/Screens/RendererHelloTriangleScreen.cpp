#include "RendererHelloTriangleScreen.hpp"

#include "src/Graphics/Renderer.hpp"
#include "src/Platform/App.hpp"

#include <utility>
#include <imgui.h>

static constexpr char const g_VertexShader[] =
R"(
    #version 330 core

    in vec3 aPos;

    void main()
    {
        gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
    }
)";

static constexpr char const g_FragmentShader[] =
R"(
    #version 330 core

    out vec4 FragColor;
    uniform vec4 uColor;

    void main()
    {
        FragColor = uColor;
    }
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
        if (e.type == SDL_QUIT)
        {
            App::upd().requestQuit();
            return;
        }
        else if (ImGuiOnEvent(e))
        {
            return;
        }
    }

    void tick(float)
    {
    }

    void draw()
    {
        App::upd().clearScreen({0.0f, 0.0f, 0.0f, 0.0f});

        ImGuiNewFrame();
        if (ImGui::Begin("panel"))
        {
            ImGui::Text("hi");
        }
        ImGui::End();
        ImGuiRender();
    }

private:
    experimental::Shader m_Shader{g_VertexShader, g_FragmentShader};
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

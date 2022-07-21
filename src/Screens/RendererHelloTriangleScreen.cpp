#include "RendererHelloTriangleScreen.hpp"

#include "src/Graphics/MeshGen.hpp"
#include "src/Graphics/Renderer.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Log.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Widgets/LogViewer.hpp"

#include <glm/vec4.hpp>
#include <imgui.h>

#include <string>
#include <utility>
#include <vector>

static char g_VertexShader[] =
R"(
    #version 330 core

    uniform mat4 uProjMat;
    uniform mat4 uViewMat;
    uniform mat4 uModelMat;

    layout (location = 0) in vec3 aPos;

    void main()
    {
        gl_Position = uProjMat * uViewMat * uModelMat * vec4(aPos, 1.0);
    }
)";

static char const g_FragmentShader[] =
R"(
    #version 330 core

    uniform vec4 uColor;

    out vec4 FragColor;

    void main()
    {
        FragColor = uColor;
    }
)";

static osc::experimental::Mesh GenerateTriangleMesh()
{
    osc::experimental::Mesh m;
    std::vector<glm::vec3> trianglePoints =
    {
        {-1.0f, -1.0f, 0.0f},  // bottom-left
        { 1.0f, -1.0f, 0.0f},  // bottom-right
        { 0.0f,  1.0f, 0.0f},  // top-middle
    };
    std::vector<std::uint16_t> indices = {0, 1, 2};

    m.setVerts(trianglePoints);
    m.setIndices(indices);
    // no tex coords or normals
    return m;
}

class osc::RendererHelloTriangleScreen::Impl final {
public:
    Impl()
    {
        m_Camera.setBackgroundColor({ 1.0f, 0.0f, 0.0f, 0.0f });
        m_Camera.setPosition({ 0.0f, 0.0f, 1.0f });
        m_Camera.setDirection({ 0.0f, 0.0f, -1.0f });
        m_Camera.setCameraProjection(osc::experimental::CameraProjection::Orthographic);
        m_Camera.setOrthographicSize(1.0f);
        m_Camera.setNearClippingPlane(0.0f);
        m_Camera.setFarClippingPlane(2.0f);

        log::info("---shader---");
        log::info("%s", StreamToString(m_Shader).c_str());
        log::info("---/shader---");

        log::info("---material---");
        log::info("%s", StreamToString(m_Material).c_str());
        log::info("---/material---");

        log::info("---mesh---");
        log::info("%s", StreamToString(m_TriangleMesh).c_str());
        log::info("---/mesh---");

        log::info("---camera---");
        log::info("%s", StreamToString(m_Camera).c_str());
        log::info("---/camera---");
    }

    void onMount()
    {
        osc::App::upd().makeMainEventLoopPolling();
        ImGuiInit();
    }

    void onUnmount()
    {
        ImGuiShutdown();
        osc::App::upd().makeMainEventLoopWaiting();
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

    void onTick()
    {
    }

    void onDraw()
    {
        App::upd().clearScreen({0.0f, 0.0f, 0.0f, 0.0f});

        osc::experimental::Graphics::DrawMesh(m_TriangleMesh, osc::Transform{}, m_Material, m_Camera);
        m_Camera.render();

        ImGuiNewFrame();
        if (ImGui::Begin("panel"))
        {
            ImGui::Text("hi");
        }
        ImGui::End();

        ImGui::Begin("log");
        m_LogViewer.draw();
        ImGui::End();

        ImGuiRender();
    }

private:
    experimental::Shader m_Shader{g_VertexShader, g_FragmentShader};
    experimental::Material m_Material{m_Shader};
    experimental::Mesh m_TriangleMesh = GenerateTriangleMesh();
    experimental::Camera m_Camera;
    LogViewer m_LogViewer;

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

void osc::RendererHelloTriangleScreen::onTick()
{
    m_Impl->onTick();
}

void osc::RendererHelloTriangleScreen::onDraw()
{
    m_Impl->onDraw();
}

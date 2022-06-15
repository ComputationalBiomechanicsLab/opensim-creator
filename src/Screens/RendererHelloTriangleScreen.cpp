#include "RendererHelloTriangleScreen.hpp"

#include "src/Graphics/Renderer.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Log.hpp"
#include "src/Widgets/LogViewer.hpp"

#include <utility>
#include <imgui.h>

static char g_VertexShader[] =
R"(
    #version 330 core

    uniform mat4 uProjMat;
    uniform mat4 uViewMat;
    uniform vec3 uLightDir;
    uniform vec3 uLightColor;
    uniform vec3 uViewPos;

    layout (location = 0) in vec3 aPos;
    layout (location = 2) in vec3 aNormal;

    layout (location = 6) in mat4x3 aModelMat;
    layout (location = 10) in mat3 aNormalMat;
    layout (location = 13) in vec4 aRgba0;

    out vec4 GouraudBrightness;
    out vec4 Rgba0;

    const float ambientStrength = 0.7f;
    const float diffuseStrength = 0.3f;
    const float specularStrength = 0.1f;
    const float shininess = 32;

    void main()
    {
        mat4 modelMat = mat4(vec4(aModelMat[0], 0), vec4(aModelMat[1], 0), vec4(aModelMat[2], 0), vec4(aModelMat[3], 1));

        gl_Position = uProjMat * uViewMat * modelMat * vec4(aPos, 1.0);

        vec3 normalDir = normalize(aNormalMat * aNormal);
        vec3 fragPos = vec3(modelMat * vec4(aPos, 1.0));
        vec3 frag2viewDir = normalize(uViewPos - fragPos);
        vec3 frag2lightDir = normalize(-uLightDir);  // light dir is in the opposite direction

        vec3 ambientComponent = ambientStrength * uLightColor;

        float diffuseAmount = max(dot(normalDir, frag2lightDir), 0.0);
        vec3 diffuseComponent = diffuseStrength * diffuseAmount * uLightColor;

        vec3 halfwayDir = normalize(frag2lightDir + frag2viewDir);
        float specularAmmount = pow(max(dot(normalDir, halfwayDir), 0.0), shininess);
        vec3 specularComponent = specularStrength * specularAmmount * uLightColor;

        vec3 lightStrength = ambientComponent + diffuseComponent + specularComponent;

        GouraudBrightness = vec4(uLightColor * lightStrength, 1.0);
        Rgba0 = aRgba0;
    }
)";

static char const g_FragmentShader[] =
R"(
    #version 330 core

    in vec4 GouraudBrightness;
    in vec4 Rgba0;

    out vec4 ColorOut;

    void main()
    {
        ColorOut = GouraudBrightness * Rgba0;
    }
)";


class osc::RendererHelloTriangleScreen::Impl final {
public:
    Impl()
    {
        log::info("%s", osc::experimental::to_string(m_Shader).c_str());
    }

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

    void tick()
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

        m_LogViewer.draw();

        ImGuiRender();
    }

private:
    experimental::Shader m_Shader{g_VertexShader, g_FragmentShader};
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

void osc::RendererHelloTriangleScreen::tick()
{
    m_Impl->tick();
}

void osc::RendererHelloTriangleScreen::draw()
{
    m_Impl->draw();
}

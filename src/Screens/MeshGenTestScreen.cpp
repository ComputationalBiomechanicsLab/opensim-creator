#include "MeshGenTestScreen.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/BasicRendererParams.hpp"
#include "src/Graphics/BasicSceneElement.hpp"
#include "src/Graphics/MeshCache.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/Maths/PolarPerspectiveCamera.hpp"
#include "src/Platform/App.hpp"
#include "src/Screens/ExperimentsScreen.hpp"
#include "src/Widgets/BasicSceneViewer.hpp"

#include <glm/gtx/transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <imgui.h>
#include <SDL_events.h>

#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

namespace osc
{
    class Mesh;
}

static std::map<std::string, std::shared_ptr<osc::Mesh>> GenerateMeshLookup()
{
    return
    {
        {"sphere", osc::App::meshes().getSphereMesh()},
        {"cylinder", osc::App::meshes().getCylinderMesh()},
        {"brick", osc::App::meshes().getBrickMesh()},
        {"cone", osc::App::meshes().getConeMesh()},
        {"floor", osc::App::meshes().getFloorMesh()},
        {"100x100 grid", osc::App::meshes().get100x100GridMesh()},
        {"cube (wire)", osc::App::meshes().getCubeWireMesh()},
        {"yline", osc::App::meshes().getYLineMesh()},
        {"quad", osc::App::meshes().getTexturedQuadMesh()},
    };
}

class osc::MeshGenTestScreen::Impl final {
public:
    Impl()
    {
        m_Camera.radius = 5.0f;
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
        }
        else if (osc::ImGuiOnEvent(e))
        {
            return;  // ImGui handled this particular event
        }
        else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
        {
            App::upd().requestTransition<ExperimentsScreen>();
            return;
        }
    }

    void onTick()
    {
    }

    void onDraw()
    {
        osc::ImGuiNewFrame();
        App::upd().clearScreen({});

        if (m_Viewer.isHovered())
        {
            osc::UpdatePolarCameraFromImGuiUserInput(osc::App::get().dims(), m_Camera);
        }

        if (ImGui::Begin("viewer"))
        {
            glm::vec2 pos = ImGui::GetCursorPos();
            for (auto const& [name, _] : m_AllMeshes)
            {
                if (ImGui::Button(name.c_str()))
                {
                    m_CurrentMesh = name;
                }
                ImGui::SameLine();
            }
            ImGui::SetCursorPos(pos);

            glm::vec2 contentRegion = ImGui::GetContentRegionAvail();
            m_Viewer.setDimensions(contentRegion);
            m_Viewer.setSamples(osc::App::get().getMSXAASamplesRecommended());

            {
                glm::vec3 p = glm::normalize(-m_Camera.focusPoint - m_Camera.getPos());
                glm::vec3 up = {0.0f, 1.0f, 0.0f};
                glm::vec3 mp = glm::rotate(glm::mat4{1.0f}, 1.25f * fpi4, up) * glm::vec4{p, 0.0f};

                m_RenderParams.lightDirection = glm::normalize(mp + -up);
                m_RenderParams.projectionMatrix = m_Camera.getProjMtx(AspectRatio(m_Viewer.getDimensions()));
                m_RenderParams.viewMatrix = m_Camera.getViewMtx();
                m_RenderParams.viewPos = m_Camera.getPos();

                BasicSceneElement bse{ {}, {1.0f, 1.0f, 1.0f, 1.0f}, m_AllMeshes[m_CurrentMesh]};
                m_Viewer.draw(m_RenderParams, {&bse, 1});
            }
        }
        ImGui::End();

        osc::ImGuiRender();
    }

private:
    std::string m_CurrentMesh = "brick";
    std::map<std::string, std::shared_ptr<osc::Mesh>> m_AllMeshes = GenerateMeshLookup();
    BasicSceneViewer m_Viewer;
    BasicRendererParams m_RenderParams;
    PolarPerspectiveCamera m_Camera;
};

osc::MeshGenTestScreen::MeshGenTestScreen() :
    m_Impl{new Impl{}}
{
}

osc::MeshGenTestScreen::MeshGenTestScreen(MeshGenTestScreen&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::MeshGenTestScreen& osc::MeshGenTestScreen::operator=(MeshGenTestScreen&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::MeshGenTestScreen::~MeshGenTestScreen() noexcept
{
    delete m_Impl;
}

void osc::MeshGenTestScreen::onMount()
{
    m_Impl->onMount();
}

void osc::MeshGenTestScreen::onUnmount()
{
    m_Impl->onUnmount();
}

void osc::MeshGenTestScreen::onEvent(SDL_Event const& e)
{
    m_Impl->onEvent(e);
}

void osc::MeshGenTestScreen::onTick()
{
    m_Impl->onTick();
}

void osc::MeshGenTestScreen::onDraw()
{
    m_Impl->onDraw();
}

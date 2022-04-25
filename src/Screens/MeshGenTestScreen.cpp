#include "MeshGenTestScreen.hpp"

#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/MeshCache.hpp"
#include "src/Platform/App.hpp"
#include "src/Widgets/BasicSceneViewer.hpp"

#include <imgui.h>
#include <SDL_events.h>

#include <map>
#include <memory>
#include <utility>

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
            App::cur().requestQuit();
        }

        ImGuiOnEvent(e);
    }

    void tick(float) {}

    void draw()
    {
        osc::ImGuiNewFrame();
        App::cur().clearScreen({});

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
            m_Viewer.setSamples(osc::App::cur().getMSXAASamplesRecommended());
            BasicSceneElement bse{ {}, {1.0f, 1.0f, 1.0f, 1.0f}, m_AllMeshes[m_CurrentMesh]};
            m_Viewer.draw({&bse, 1});
        }
        ImGui::End();

        osc::ImGuiRender();
    }

private:
    std::string m_CurrentMesh = "brick";
    std::map<std::string, std::shared_ptr<osc::Mesh>> m_AllMeshes = GenerateMeshLookup();
    BasicSceneViewer m_Viewer;
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

void osc::MeshGenTestScreen::tick(float dt)
{
    m_Impl->tick(dt);
}

void osc::MeshGenTestScreen::draw()
{
    m_Impl->draw();
}

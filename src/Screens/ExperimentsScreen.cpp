#include "ExperimentsScreen.hpp"

#include "src/Platform/App.hpp"
#include "src/Screens/HelloTriangleScreen.hpp"
#include "src/Screens/HittestScreen.hpp"
#include "src/Screens/ImGuizmoDemoScreen.hpp"
#include "src/Screens/ImGuiDemoScreen.hpp"
#include "src/Screens/InstancedRendererScreen.hpp"
#include "src/Screens/MathExperimentsScreen.hpp"
#include "src/Screens/MeshGenTestScreen.hpp"
#include "src/Screens/MeshHittestScreen.hpp"
#include "src/Screens/MeshHittestWithBVHScreen.hpp"
#include "src/Screens/MeshImporterScreen.hpp"
#include "src/Screens/SplashScreen.hpp"

#include <imgui.h>

#include <string>
#include <vector>

template<typename Screen>
static void transition()
{
    osc::App::cur().requestTransition<Screen>();
}

using transition_fn = void(*)(void);
struct Entry final { std::string name; transition_fn f;  };

// experiments screen impl
class osc::ExperimentsScreen::Impl final {
public:
    void onMount()
    {
        osc::ImGuiInit();
    }

    void onUnmount()
    {
        osc::ImGuiShutdown();
    }

    void onEvent(SDL_Event const& e)
    {
        if (e.type == SDL_QUIT)
        {
            App::cur().requestQuit();
            return;
        }
        else if (osc::ImGuiOnEvent(e))
        {
            return;  // ImGui handled it
        }
        else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
        {
            App::cur().requestTransition<SplashScreen>();
            return;
        }
    }

    void draw()
    {
        App::cur().clearScreen({0.0f, 0.0f, 0.0f, 0.0f});
        osc::ImGuiNewFrame();

        glm::vec2 dims = App::cur().dims();
        glm::vec2 menuDims = {700.0f, 500.0f};

        // center the menu
        {
            glm::vec2 menu_pos = (dims - menuDims) / 2.0f;
            ImGui::SetNextWindowPos(menu_pos);
            ImGui::SetNextWindowSize(ImVec2(menuDims.x, -1));
            ImGui::SetNextWindowSizeConstraints(menuDims, menuDims);
        }

        ImGui::Begin("select experiment");

        for (auto const& e : m_Entries)
        {
            if (ImGui::Button(e.name.c_str()))
            {
                e.f();
            }
        }

        ImGui::End();

        osc::ImGuiRender();
    }

private:
    std::vector<Entry> m_Entries =
    {
        { "Hello Triangle (OpenGL test)", transition<HelloTriangleScreen> },
        { "Hit testing analytical geometry (AABBs, Spheres, etc.)", transition<HittestScreen> },
        { "Hit testing ray-triangle intersections in a mesh", transition<MeshHittestScreen> },
        { "Random math experiments", transition<MathExperimentsScreen> },
        { "Hit testing ray-triangle, but with BVH acceleration", transition<MeshHittestWithBVHScreen> },
        { "OpenSim mesh importer wizard", transition<MeshImporterScreen> },
        { "Instanced rendering", transition<InstancedRendererScreen> },
        { "ImGuizmo", transition<ImGuizmoDemoScreen> },
        { "ImGui", transition<ImGuiDemoScreen> },
        { "Mesh Generation Tests", transition<MeshGenTestScreen> },
    };
};

// public API (PIMPL)

osc::ExperimentsScreen::ExperimentsScreen() :
    m_Impl{new Impl{}}
{
}

osc::ExperimentsScreen::ExperimentsScreen(ExperimentsScreen&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::ExperimentsScreen& osc::ExperimentsScreen::operator=(ExperimentsScreen&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::ExperimentsScreen::~ExperimentsScreen() noexcept
{
    delete m_Impl;
}

void osc::ExperimentsScreen::onMount()
{
    m_Impl->onMount();
}

void osc::ExperimentsScreen::onUnmount()
{
    m_Impl->onUnmount();
}

void osc::ExperimentsScreen::onEvent(SDL_Event const& e)
{
    m_Impl->onEvent(e);
}

void osc::ExperimentsScreen::draw()
{
    m_Impl->draw();
}

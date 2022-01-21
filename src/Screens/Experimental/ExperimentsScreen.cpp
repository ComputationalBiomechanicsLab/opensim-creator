#include "ExperimentsScreen.hpp"

#include "src/3D/Gl.hpp"
#include "src/Screens/Experimental/HelloTriangleScreen.hpp"
#include "src/Screens/Experimental/HittestScreen.hpp"
#include "src/Screens/Experimental/ImGuizmoDemoScreen.hpp"
#include "src/Screens/Experimental/ImGuiDemoScreen.hpp"
#include "src/Screens/Experimental/InstancedRendererScreen.hpp"
#include "src/Screens/Experimental/LayeredInterfaceScreen.hpp"
#include "src/Screens/Experimental/MathExperimentsScreen.hpp"
#include "src/Screens/Experimental/MeshHittestScreen.hpp"
#include "src/Screens/Experimental/MeshHittestWithBVHScreen.hpp"
#include "src/Screens/MeshImporterScreen.hpp"
#include "src/Screens/SplashScreen.hpp"
#include "src/App.hpp"

#include <imgui.h>

#include <string>
#include <vector>

using namespace osc;

template<typename Screen>
static void transition()
{
    App::cur().requestTransition<Screen>();
}

using transition_fn = void(*)(void);
struct Entry final { std::string name; transition_fn f;  };

// experiments screen impl
struct ExperimentsScreen::Impl final {
    std::vector<Entry> entries =
    {
        { "Hello Triangle (OpenGL test)", transition<HelloTriangleScreen> },
        { "Hit testing analytical geometry (AABBs, Spheres, etc.)", transition<HittestScreen> },
        { "Hit testing ray-triangle intersections in a mesh", transition<MeshHittestScreen> },
        { "Random math experiments", transition<MathExperimentsScreen> },
        { "Hit testing ray-triangle, but with BVH acceleration", transition<MeshHittestWithBVHScreen> },
        { "OpenSim mesh importer wizard", transition<MeshImporterScreen> },
        { "Instanced rendering", transition<InstancedRendererScreen> },
        { "Layered Interface", transition<LayeredInterfaceScreen> },
        { "ImGuizmo", transition<ImGuizmoDemoScreen> },
        { "ImGui", transition<ImGuiDemoScreen> }
    };
};

// public API

osc::ExperimentsScreen::ExperimentsScreen() :
    m_Impl{new Impl{}}
{
}

osc::ExperimentsScreen::~ExperimentsScreen() noexcept = default;

void osc::ExperimentsScreen::onMount()
{
    osc::ImGuiInit();
}

void osc::ExperimentsScreen::onUnmount()
{
    osc::ImGuiShutdown();
}

void osc::ExperimentsScreen::onEvent(SDL_Event const& e)
{
    if (osc::ImGuiOnEvent(e))
    {
        return;
    }

    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
    {
        App::cur().requestTransition<SplashScreen>();
        return;
    }
}

void ExperimentsScreen::draw()
{
    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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

    for (auto const& e : m_Impl->entries)
    {
        if (ImGui::Button(e.name.c_str()))
        {
            e.f();
        }
    }

    ImGui::End();

    osc::ImGuiRender();
}

#include "ExperimentsScreen.hpp"

#include "src/3d/Gl.hpp"
#include "src/screens/experimental/Component3DViewerScreen.hpp"
#include "src/screens/experimental/HelloTriangleScreen.hpp"
#include "src/screens/experimental/HittestScreen.hpp"
#include "src/screens/experimental/MeshHittestWithBVHScreen.hpp"
#include "src/screens/experimental/InstancedRendererScreen.hpp"
#include "src/screens/experimental/MeshHittestScreen.hpp"
#include "src/screens/experimental/MeshesToModelWizardScreen.hpp"
#include "src/screens/experimental/OpenSimModelstateDecorationGeneratorScreen.hpp"
#include "src/screens/experimental/SimbodyMeshgenScreen.hpp"
#include "src/screens/experimental/ImGuizmoDemoScreen.hpp"
#include "src/screens/SplashScreen.hpp"
#include "src/App.hpp"

#include <imgui.h>

#include <string>
#include <vector>

using namespace osc;

template<typename Screen>
static void transition() {
    App::cur().requestTransition<Screen>();
}

using transition_fn = void(*)(void);
struct Entry final { std::string name; transition_fn f;  };

// experiments screen impl
struct ExperimentsScreen::Impl final {
    std::vector<Entry> entries = {
        { "Component 3D Viewer Test", transition<Component3DViewerScreen> },
        { "Hello Triangle (OpenGL test)", transition<HelloTriangleScreen> },
        { "Hit testing analytical geometry (AABBs, Spheres, etc.)", transition<HittestScreen> },
        { "Hit testing ray-triangle intersections in a mesh", transition<MeshHittestScreen> },
        { "Hit testing ray-triangle, but with BVH acceleration", transition<MeshHittestWithBVHScreen> },
        { "OpenSim mesh importer wizard", transition<MeshesToModelWizardScreen> },
        { "OpenSim Model+State decoration generation", transition<OpenSimModelstateDecorationGeneratorScreen> },
        { "Simbody mesh generation", transition<SimbodyMeshgenScreen> },
        { "Instanced rendering", transition<InstancedRendererScreen> },
        { "ImGuizmo", transition<ImGuizmoDemoScreen> },
    };
};

// public API

osc::ExperimentsScreen::ExperimentsScreen() : m_Impl{new Impl{}} {
}

osc::ExperimentsScreen::~ExperimentsScreen() noexcept = default;

void osc::ExperimentsScreen::onMount() {
    osc::ImGuiInit();
}

void osc::ExperimentsScreen::onUnmount() {
    osc::ImGuiShutdown();
}

void osc::ExperimentsScreen::onEvent(SDL_Event const& e) {
    if (osc::ImGuiOnEvent(e)) {
        return;
    }

    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        App::cur().requestTransition<SplashScreen>();
        return;
    }
}

void ExperimentsScreen::draw() {
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

    for (auto const& e : m_Impl->entries) {
        if (ImGui::Button(e.name.c_str())) {
            e.f();
        }
    }

    ImGui::End();

    osc::ImGuiRender();
}

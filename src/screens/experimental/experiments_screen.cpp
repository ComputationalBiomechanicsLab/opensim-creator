#include "experiments_screen.hpp"

#include "src/3d/gl.hpp"
#include "src/screens/experimental/component_3d_viewer_screen.hpp"
#include "src/screens/experimental/hellotriangle_screen.hpp"
#include "src/screens/experimental/hittest_screen.hpp"
#include "src/screens/experimental/mesh_hittest_with_bvh_screen.hpp"
#include "src/screens/experimental/instanced_renderer_screen.hpp"
#include "src/screens/experimental/mesh_hittest_screen.hpp"
#include "src/screens/experimental/meshes_to_model_wizard_screen.hpp"
#include "src/screens/experimental/opensim_modelstate_decoration_generator_screen.hpp"
#include "src/screens/experimental/simbody_meshgen_screen.hpp"
#include "src/screens/experimental/imguizmo_demo_screen.hpp"
#include "src/screens/splash_screen.hpp"
#include "src/app.hpp"

#include <imgui.h>

#include <string>
#include <vector>

using namespace osc;

template<typename Screen>
static void transition() {
    App::cur().request_transition<Screen>();
}

using transition_fn = void(*)(void);
struct Entry final { std::string name; transition_fn f;  };

// experiments screen impl
struct Experiments_screen::Impl final {
    std::vector<Entry> entries = {
        { "Component 3D Viewer Test", transition<Component_3d_viewer_screen> },
        { "Hello Triangle (OpenGL test)", transition<Hellotriangle_screen> },
        { "Hit testing analytical geometry (AABBs, Spheres, etc.)", transition<Hittest_screen> },
        { "Hit testing ray-triangle intersections in a mesh", transition<Mesh_hittesting> },
        { "Hit testing ray-triangle, but with BVH acceleration", transition<Mesh_hittest_with_bvh_screen> },
        { "OpenSim mesh importer wizard", transition<Meshes_to_model_wizard_screen> },
        { "OpenSim Model+State decoration generation", transition<Opensim_modelstate_decoration_generator_screen> },
        { "Simbody mesh generation", transition<Simbody_meshgen_screen> },
        { "Instanced rendering", transition<Instanced_render_screen> },
        { "ImGuizmo", transition<Imguizmo_demo_screen> },
    };
};

// public API

osc::Experiments_screen::Experiments_screen() : impl{new Impl{}} {
}

osc::Experiments_screen::~Experiments_screen() noexcept = default;

void osc::Experiments_screen::on_mount() {
    osc::ImGuiInit();
}

void osc::Experiments_screen::on_unmount() {
    osc::ImGuiShutdown();
}

void osc::Experiments_screen::on_event(SDL_Event const& e) {
    if (osc::ImGuiOnEvent(e)) {
        return;
    }

    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        App::cur().request_transition<Splash_screen>();
        return;
    }
}

void Experiments_screen::draw() {
    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    osc::ImGuiNewFrame();

    glm::vec2 dims = App::cur().dims();
    glm::vec2 menu_dims = {700.0f, 500.0f};

    // center the menu
    {
        glm::vec2 menu_pos = (dims - menu_dims) / 2.0f;
        ImGui::SetNextWindowPos(menu_pos);
        ImGui::SetNextWindowSize(ImVec2(menu_dims.x, -1));
        ImGui::SetNextWindowSizeConstraints(menu_dims, menu_dims);
    }

    ImGui::Begin("select experiment");

    for (auto const& e : impl->entries) {
        if (ImGui::Button(e.name.c_str())) {
            e.f();
        }
    }

    ImGui::End();

    osc::ImGuiRender();
}

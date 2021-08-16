#include "experiments_screen.hpp"

#include "src/app.hpp"
#include "src/3d/gl.hpp"
#include "src/screens/experimental/hellotriangle_screen.hpp"
#include "src/screens/experimental/hittest_screen.hpp"
#include "src/screens/experimental/mesh_hittest_with_bvh_screen.hpp"
#include "src/screens/experimental/instanced_renderer_screen.hpp"
#include "src/screens/experimental/mesh_hittest_screen.hpp"
#include "src/screens/experimental/simbody_meshgen_screen.hpp"
#include "src/screens/experimental/imguizmo_demo_screen.hpp"

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
        { "Hello triangle", transition<Hellotriangle_screen> },
        { "Basic hit testing", transition<Hittest_screen> },
        { "Instanced rendering", transition<Instanced_render_screen> },
        { "Mesh hittesting", transition<Mesh_hittesting> },
        { "Mesh hittesting with basic BVH", transition<Mesh_hittest_with_bvh_screen> },
        { "OpenSim mesh generation", transition<Simbody_meshgen_screen> },
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
        // TODO
        //Application::current().request_transition<osc::Splash_screen>();
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

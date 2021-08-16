#include "opensim_modelstate_decoration_generator_screen.hpp"

#include "src/app.hpp"
#include "src/3d/gl.hpp"
#include "src/3d/instanced_renderer.hpp"
#include "src/opensim_bindings/scene_generator.hpp"
#include "src/utils/perf.hpp"

#include <imgui.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <SimTKcommon.h>

using namespace osc;

struct osc::Opensim_modelstate_decoration_generator_screen::Impl final {
    Instanced_renderer renderer;
    Render_params render_params;
    Scene_generator generator{renderer};
    Scene_decorations scene_decorations;
    OpenSim::Model model{App::resource("models/ToyLanding/ToyLandingModel.osim").string()};
    SimTK::State state = [this]() {
        model.finalizeFromProperties();
        model.finalizeConnections();
        SimTK::State s = model.initSystem();
        model.realizeReport(s);
        return s;
    }();
    Polar_perspective_camera camera;

    Basic_perf_timer timer_meshgen;
    Basic_perf_timer timer_sort;
    Basic_perf_timer timer_render;
    Basic_perf_timer timer_blit;
};

// public API

osc::Opensim_modelstate_decoration_generator_screen::Opensim_modelstate_decoration_generator_screen() :
    m_Impl{new Impl{}} {

    App::cur().disable_vsync();
}

osc::Opensim_modelstate_decoration_generator_screen::~Opensim_modelstate_decoration_generator_screen() noexcept = default;

void osc::Opensim_modelstate_decoration_generator_screen::on_mount() {
    osc::ImGuiInit();
}

void osc::Opensim_modelstate_decoration_generator_screen::on_unmount() {
    osc::ImGuiShutdown();
}

void osc::Opensim_modelstate_decoration_generator_screen::on_event(SDL_Event const& e) {
    if (osc::ImGuiOnEvent(e)) {
        return;
    }
}

void osc::Opensim_modelstate_decoration_generator_screen::draw() {
    osc::ImGuiNewFrame();

    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Impl& s = *m_Impl;

    // camera stuff
    {
        Impl& impl = *m_Impl;

        impl.camera.radius *= 1.0f - ImGui::GetIO().MouseWheel/10.0f;

        // handle panning/zooming/dragging with middle mouse
        if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {

            // in pixels, e.g. [800, 600]
            glm::vec2 screendims = App::cur().dims();

            // in pixels, e.g. [-80, 30]
            glm::vec2 mouse_delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle, 0.0f);
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Middle);

            // as a screensize-independent ratio, e.g. [-0.1, 0.05]
            glm::vec2 relative_delta = mouse_delta / screendims;

            if (ImGui::IsKeyDown(SDL_SCANCODE_LSHIFT) || ImGui::IsKeyDown(SDL_SCANCODE_RSHIFT)) {
                // shift + middle-mouse performs a pan
                float aspect_ratio = screendims.x / screendims.y;
                impl.camera.do_pan(aspect_ratio, relative_delta);
            } else if (ImGui::IsKeyDown(SDL_SCANCODE_LCTRL) || ImGui::IsKeyDown(SDL_SCANCODE_RCTRL)) {
                // shift + middle-mouse performs a zoom
                impl.camera.radius *= 1.0f + relative_delta.y;
            } else {
                // just middle-mouse performs a mouse drag
                impl.camera.do_drag(relative_delta);
            }
        }
    }

    s.model.updDisplayHints().set_show_frames(true);
    s.model.updDisplayHints().set_show_wrap_geometry(true);

    // generate decorations for model

    {
        auto guard = s.timer_meshgen.measure();
        s.generator.generate(s.renderer, s.model, s.state, s.model.getDisplayHints(), s.scene_decorations);
    }

    {
        auto guard = s.timer_sort.measure();
        std::sort(s.scene_decorations.drawlist.instances.begin(), s.scene_decorations.drawlist.instances.end(), [](Mesh_instance const& mi1, Mesh_instance const& mi2) {
            return mi1.meshidx < mi2.meshidx;
        });
    }

    // render the decorations into the renderer's texture
    s.renderer.set_dims(App::cur().idims());
    s.renderer.set_msxaa_samples(App::cur().get_samples());
    s.render_params.projection_matrix = s.camera.projection_matrix(s.renderer.aspect_ratio());
    s.render_params.view_matrix = s.camera.view_matrix();
    s.render_params.view_pos = s.camera.pos();

    {
        auto guard = s.timer_render.measure();
        s.renderer.render(s.render_params, s.scene_decorations.drawlist);
    }

    // get the texture
    {
        auto guard = s.timer_blit.measure();

        gl::Texture_2d const& render = s.renderer.output_texture();

        // draw texture using ImGui::Image
        ImGui::SetNextWindowPos({0.0f, 0.0f});
        ImGui::SetNextWindowSize(App::cur().dims());
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowBorderSize = 0.0f;
        style.WindowPadding = {0.0f, 0.0f};
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_Text] = {1.0f, 0.0f, 0.0f, 1.0f};
        ImGui::Begin("render output", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoSavedSettings);
        void* texture_handle = reinterpret_cast<void*>(static_cast<uintptr_t>(render.get()));
        ImVec2 image_dimensions{App::cur().dims()};
        ImVec2 uv0{0, 1};
        ImVec2 uv1{1, 0};
        ImGui::Image(texture_handle, image_dimensions, uv0, uv1);
        ImGui::SetCursorPos({0.0f, 0.0f});
        ImGui::Text("FPS = %.2f", ImGui::GetIO().Framerate);
        ImGui::Text("meshgen (ms) = %.2f", s.timer_meshgen.millis());
        ImGui::Text("sort (ms) = %.2f", s.timer_sort.millis());
        ImGui::Text("render (ms) = %.2f", s.timer_render.millis());
        ImGui::Text("blit (ms) = %.2f", s.timer_blit.millis());

        ImGui::End();
    }


    osc::ImGuiRender();
}

#pragma once

#include "SDL_events.h"
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace SimTK {
    class State;
}

namespace OpenSim {
    class Model;
    class Component;
}

namespace osmv {
    class Application;

    // a renderer that draws an OpenSim::Model + SimTK::State pair into the current
    // framebuffer using a basic polar camera that can swivel around the model
    struct Simple_model_renderer_impl;
    struct Simple_model_renderer final {
        // camera parameters
        //
        // perspective camera using polar coordinates for spinning around the model
        float radius = 5.0f;
        float theta = 0.88f;
        float phi = 0.4f;
        glm::vec3 pan = {0.3f, -0.5f, 0.0f};
        float fov = 120.0f;
        float znear = 0.1f;
        float zfar = 100.0f;

        // event parameters
        bool dragging = false;
        bool panning = false;
        float mouse_wheel_sensitivity = 0.9f;
        float mouse_drag_sensitivity = 1.0f;

        glm::vec3 light_pos = {1.5f, 3.0f, 0.0f};
        glm::vec3 light_rgb = {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f};
        glm::vec4 background_rgba = {0.89f, 0.89f, 0.89f, 1.0f};
        bool wireframe_mode = false;
        bool show_mesh_normals = false;
        bool show_floor = true;

        bool draw_rims = true;
        glm::vec4 rim_rgba = {1.0f, 0.4f, 0.0f, 1.0f};
        float rim_thickness = 0.002f;

        // this is set whenever the implementation detects that the mouse is over
        // a component
        OpenSim::Component const* hovered_component = nullptr;

    private:
        Simple_model_renderer_impl* impl;

    public:
        Simple_model_renderer(int w, int h, int samples);
        Simple_model_renderer(Simple_model_renderer const&) = delete;
        Simple_model_renderer(Simple_model_renderer&&) = delete;
        Simple_model_renderer& operator=(Simple_model_renderer const&) = delete;
        Simple_model_renderer& operator=(Simple_model_renderer&&) = delete;
        ~Simple_model_renderer() noexcept;

        // handle event (probably forwarded from a screen)
        //
        // returns `true` if the event was handled, `false` otherwise
        bool on_event(Application&, SDL_Event const&);

        // draw the model in the supplied state onto the screen
        void draw(
            Application const& app,
            OpenSim::Model const& model,
            SimTK::State const& st,
            OpenSim::Component const* selected = nullptr);
    };
}

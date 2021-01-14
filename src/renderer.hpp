#pragma once

#include "sdl.hpp"

#include <glm/vec3.hpp>

#include <memory>

namespace SimTK {
    class State;
}

namespace OpenSim {
    class Model;
}

namespace osmv {
    class Application;

    struct Renderer_private_state;
    struct Renderer final {
        std::unique_ptr<Renderer_private_state> state;

        float radius = 5.0f;
        float theta = 0.88f;
        float phi = 0.4f;
        glm::vec3 pan = {0.3f, -0.5f, 0.0f};
        float fov = 120.0f;
        bool dragging = false;
        bool panning = false;
        float sensitivity = 1.0f;
        glm::vec3 light_pos = {1.5f, 3.0f, 0.0f};
        glm::vec3 light_color = {0.9607f, 0.9176f, 0.8863f};
        bool wireframe_mode = false;
        bool show_light = false;
        bool show_unit_cylinder = false;
        bool gamma_correction = false;
        bool show_mesh_normals = false;
        bool show_floor = true;
        float wheel_sensitivity = 0.9f;

        Renderer();
        Renderer(Renderer const&) = delete;
        Renderer(Renderer&&) = delete;
        Renderer& operator=(Renderer const&) = delete;
        Renderer& operator=(Renderer&&) = delete;
        ~Renderer() noexcept;

        bool on_event(Application&, SDL_Event const&);
        void draw(Application const&, OpenSim::Model&, SimTK::State&);
    };
}

#pragma once

#include <SDL_events.h>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace OpenSim {
    class Component;
}
namespace OpenSim {
    class Model;
}
namespace SimTK {
    class State;
}
namespace osmv {
    class Application;
}
namespace osmv {
    struct Simple_model_renderer_impl;
}

namespace osmv {
    using SimpleModelRendererFlags = int;

    enum SimpleModelRendererFlags_ {
        SimpleModelRendererFlags_None = 0,

        // camera is in a "currently dragging" state
        SimpleModelRendererFlags_Dragging = 1 << 0,

        // camera is in a "currently panning" state
        SimpleModelRendererFlags_Panning = 1 << 1,

        // renderer should draw in wireframe mode
        SimpleModelRendererFlags_WireframeMode = 1 << 2,

        // renderer should draw mesh normals
        SimpleModelRendererFlags_ShowMeshNormals = 1 << 3,

        // renderer should draw a chequered floor
        SimpleModelRendererFlags_ShowFloor = 1 << 4,

        // renderer should draw selection rims
        SimpleModelRendererFlags_DrawRims = 1 << 5,

        SimpleModelRendererFlags_Default = SimpleModelRendererFlags_ShowFloor | SimpleModelRendererFlags_DrawRims,
    };

    // a renderer that draws an OpenSim::Model + SimTK::State pair into the current
    // framebuffer using a basic polar camera that can swivel around the model
    struct Simple_model_renderer final {
    private:
        Simple_model_renderer_impl* impl;

    public:
        // this is set whenever the implementation detects that the mouse is over
        // a component
        OpenSim::Component const* hovered_component = nullptr;

        // not currently runtime-editable
        static constexpr float fov = 120.0f;
        static constexpr float znear = 0.1f;
        static constexpr float zfar = 100.0f;
        static constexpr float mouse_wheel_sensitivity = 0.9f;
        static constexpr float mouse_drag_sensitivity = 1.0f;

        float radius = 5.0f;
        float theta = 0.88f;
        float phi = 0.4f;
        glm::vec3 pan = {0.3f, -0.5f, 0.0f};
        glm::vec3 light_pos = {1.5f, 3.0f, 0.0f};
        glm::vec3 light_rgb = {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f};
        glm::vec4 background_rgba = {0.89f, 0.89f, 0.89f, 1.0f};
        glm::vec4 rim_rgba = {1.0f, 0.4f, 0.0f, 1.0f};
        float rim_thickness = 0.002f;

        SimpleModelRendererFlags flags = SimpleModelRendererFlags_Default;

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
        bool on_event(SDL_Event const&);

        // draw the model in the supplied state onto the screen
        void draw(OpenSim::Model const& model, SimTK::State const& st, OpenSim::Component const* selected = nullptr);
    };
}

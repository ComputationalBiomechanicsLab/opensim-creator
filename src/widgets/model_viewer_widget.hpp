#pragma once

#include <SDL_events.h>

namespace OpenSim {
    class Model;
    class Component;
}

namespace SimTK {
    class State;
}

namespace osmv {
    class Polar_camera;
}

namespace osmv {
    struct Model_viewer_widget_impl;

    class Model_viewer_widget final {
        Model_viewer_widget_impl* impl;

    public:
        Model_viewer_widget();
        Model_viewer_widget(Model_viewer_widget const&) = delete;
        Model_viewer_widget(Model_viewer_widget&&) = delete;
        Model_viewer_widget& operator=(Model_viewer_widget const&) = delete;
        Model_viewer_widget& operator=(Model_viewer_widget&&) = delete;
        ~Model_viewer_widget() noexcept;

        bool is_moused_over() const noexcept;
        Polar_camera& camera() noexcept;
        bool on_event(SDL_Event const&);
        void draw(
            char const* panel_name,
            OpenSim::Model const&,
            SimTK::State const&,
            OpenSim::Component const** selected,
            OpenSim::Component const** hovered);
    };
}

#pragma once

#include <SDL_events.h>

namespace OpenSim {
    class Model;
}

namespace SimTK {
    class State;
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

        bool on_event(SDL_Event const&);
        void draw(OpenSim::Model const&, SimTK::State const&);
    };
}

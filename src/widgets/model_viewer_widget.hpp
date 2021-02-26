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
    struct Gpu_cache;
}

namespace osmv {
    using ModelViewerWidgetFlags = int;
    enum ModelViewerWidgetFlags_ {
        ModelViewerWidgetFlags_None = 0,
        ModelViewerWidgetFlags_CanOnlyInteractWithMuscles = 1 << 0,
    };

    class Model_viewer_widget final {
        struct Impl;
        Impl* impl;

    public:
        Model_viewer_widget(Gpu_cache&, ModelViewerWidgetFlags = ModelViewerWidgetFlags_None);
        ~Model_viewer_widget() noexcept;

        bool is_moused_over() const noexcept;
        bool on_event(SDL_Event const&);
        void draw(
            char const* panel_name,
            OpenSim::Model const&,
            SimTK::State const&,
            OpenSim::Component const** selected,
            OpenSim::Component const** hovered);
    };
}

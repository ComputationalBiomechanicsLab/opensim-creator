#pragma once

namespace SimTK {
    class State;
}

namespace OpenSim {
    class Component;
}

namespace osmv {
    class Selection_viewer final {
    public:
        Selection_viewer() = default;
        Selection_viewer(Selection_viewer const&) = delete;
        Selection_viewer(Selection_viewer&&) = delete;
        Selection_viewer& operator=(Selection_viewer const&) = delete;
        Selection_viewer& operator=(Selection_viewer&&) = delete;
        ~Selection_viewer() noexcept = default;

        // the draw call will set the selection pointer if an interaction happens
        // in the hierarchy UI
        void draw(SimTK::State const& state, OpenSim::Component const** selected);
    };
}

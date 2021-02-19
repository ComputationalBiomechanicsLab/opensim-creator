#pragma once

namespace OpenSim {
    class Component;
}

namespace osmv {
    class Hierarchy_viewer final {
    public:
        Hierarchy_viewer() = default;
        Hierarchy_viewer(Hierarchy_viewer const&) = delete;
        Hierarchy_viewer(Hierarchy_viewer&&) = delete;
        Hierarchy_viewer& operator=(Hierarchy_viewer const&) = delete;
        Hierarchy_viewer& operator=(Hierarchy_viewer&&) = delete;
        ~Hierarchy_viewer() noexcept = default;

        // the draw call will set the selection pointer if an interaction happens
        // in the hierarchy UI
        void draw(OpenSim::Component const* root, OpenSim::Component const** selected, OpenSim::Component const** hovered);
    };
}

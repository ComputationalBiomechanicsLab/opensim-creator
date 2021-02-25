#pragma once

#include <SDL_events.h>

#include <memory>

namespace OpenSim {
    class Model;
    class Component;
}

namespace SimTK {
    class State;
}

namespace osmv {
    class Polar_camera;
    struct Gpu_cache;
}

namespace osmv {
    using ModelViewerGeometryFlags = int;
    enum ModelViewerGeometryFlags_ {
        ModelViewerGeometryFlags_None = 0,
        ModelViewerGeometryFlags_DrawDynamicDecorations = 1 << 0,
        ModelViewerGeometryFlags_DrawStaticDecorations = 1 << 1,
        ModelViewerGeometryFlags_CanInteractWithDynamicDecorations = 1 << 2,
        ModelViewerGeometryFlags_CanInteractWithStaticDecorations = 1 << 3,
        ModelViewerGeometryFlags_CanOnlyInteractWithMuscles = 1 << 4,
        ModelViewerGeometryFlags_DrawFloor = 1 << 5,
        ModelViewerGeometryFlags_OptimizeDrawOrder = 1 << 6,

        ModelViewerGeometryFlags_Default =
            ModelViewerGeometryFlags_DrawDynamicDecorations | ModelViewerGeometryFlags_DrawStaticDecorations |
            ModelViewerGeometryFlags_CanInteractWithDynamicDecorations |
            ModelViewerGeometryFlags_CanOnlyInteractWithMuscles | ModelViewerGeometryFlags_DrawFloor |
            ModelViewerGeometryFlags_OptimizeDrawOrder
    };

    class Model_viewer_widget final {
        struct Impl;
        std::unique_ptr<Impl> impl;

    public:
        ModelViewerGeometryFlags geometry_flags = ModelViewerGeometryFlags_Default;

        Model_viewer_widget(Gpu_cache&);
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

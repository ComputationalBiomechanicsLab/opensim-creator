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

        ModelViewerWidgetFlags_DrawDynamicDecorations = 1 << 0,
        ModelViewerWidgetFlags_DrawStaticDecorations = 1 << 1,
        ModelViewerWidgetFlags_DrawFloor = 1 << 2,
        ModelViewerWidgetFlags_OptimizeDrawOrder = 1 << 3,
        ModelViewerWidgetFlags_DrawFrames = 1 << 4,
        ModelViewerWidgetFlags_DrawDebugGeometry = 1 << 5,
        ModelViewerWidgetFlags_DrawLabels = 1 << 6,

        ModelViewerWidgetFlags_CanOnlyInteractWithMuscles = 1 << 7,

        ModelViewerWidgetFlags_DefaultMuscleColoring = 1 << 8,
        ModelViewerWidgetFlags_RecolorMusclesByStrain = 1 << 9,  // overrides previous
        ModelViewerWidgetFlags_RecolorMusclesByLength = 1 << 10,  // overrides previous

        ModelViewerWidgetFlags_Default = ModelViewerWidgetFlags_DrawDynamicDecorations |
                                         ModelViewerWidgetFlags_DrawStaticDecorations |
                                         ModelViewerWidgetFlags_DrawFloor | ModelViewerWidgetFlags_OptimizeDrawOrder |
                                         ModelViewerWidgetFlags_DefaultMuscleColoring
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

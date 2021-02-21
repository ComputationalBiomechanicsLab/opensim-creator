#pragma once

#include "src/3d/raw_renderer.hpp"

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
    using ModelViewerGeometryFlags = int;
    enum ModelViewerGeometryFlags_ {
        ModelViewerGeometryFlags_None = 0,
        ModelViewerGeometryFlags_DrawDynamicDecorations = 1 << 0,
        ModelViewerGeometryFlags_DrawStaticDecorations = 1 << 1,
        ModelViewerGeometryFlags_CanInteractWithDynamicDecorations = 1 << 2,
        ModelViewerGeometryFlags_CanInteractWithStaticDecorations = 1 << 3,
        ModelViewerGeometryFlags_CanOnlyInteractWithMuscles = 1 << 4,

        ModelViewerGeometryFlags_Default = ModelViewerGeometryFlags_DrawDynamicDecorations |
                                           ModelViewerGeometryFlags_DrawStaticDecorations |
                                           ModelViewerGeometryFlags_CanInteractWithDynamicDecorations |
                                           ModelViewerGeometryFlags_CanOnlyInteractWithMuscles
    };

    enum ModelViewerRecoloring {
        ModelViewerRecoloring_None,
        ModelViewerRecoloring_Strain,
        ModelViewerRecoloring_Length,
    };

    struct Model_viewer_widget_impl;
    class Model_viewer_widget final {
        Model_viewer_widget_impl* impl;

    public:
        Raw_renderer_flags rendering_flags = RawRendererFlags_Default;
        ModelViewerGeometryFlags geometry_flags = ModelViewerGeometryFlags_Default;
        ModelViewerRecoloring recoloring = ModelViewerRecoloring_None;

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

#pragma once

#include <SDL_events.h>

#include <memory>

namespace OpenSim {
    class Model;
    class Component;
    class ModelDisplayHints;
}

namespace SimTK {
    class State;
}

namespace osc {

    // flags that toggle the viewer's behavior
    using Component3DViewerFlags = int;
    enum Component3DViewerFlags_ {

        // no flags: a basic-as-possible render
        Component3DViewerFlags_None = 0,

        // draw dynamic decorations, as defined by OpenSim (e.g. muscles)
        Component3DViewerFlags_DrawDynamicDecorations = 1 << 0,

        // draw static decorations, as defined by OpenSim (e.g. meshes)
        Component3DViewerFlags_DrawStaticDecorations = 1 << 1,

        // draw model "frames", as defined by OpenSim (e.g. body frames)
        Component3DViewerFlags_DrawFrames = 1 << 2,

        // draw debug geometry, as defined by OpenSim
        Component3DViewerFlags_DrawDebugGeometry = 1 << 3,

        // draw labels, as defined by OpenSim
        Component3DViewerFlags_DrawLabels = 1 << 4,

        // draw a 2D XZ grid
        Component3DViewerFlags_DrawXZGrid = 1 << 5,

        // draw a 2D XY grid
        Component3DViewerFlags_DrawXYGrid = 1 << 6,

        // draw a 2D YZ grid
        Component3DViewerFlags_DrawYZGrid = 1 << 7,

        // draw axis lines (the red/green lines on the floor showing axes)
        Component3DViewerFlags_DrawAxisLines = 1 << 8,

        // draw AABBs (debugging)
        Component3DViewerFlags_DrawAABBs = 1 << 9,

        // draw scene BVH (debugging)
        Component3DViewerFlags_DrawBVH = 1 << 10,

        // draw alignment axes
        //
        // these are little red+green+blue demo axes in corner of the viewer that
        // show the user how the world axes align relative to the current view location
        Component3DViewerFlags_DrawAlignmentAxes = 1 << 11,

        Component3DViewerFlags_Default = Component3DViewerFlags_DrawDynamicDecorations |
                                         Component3DViewerFlags_DrawStaticDecorations,
    };

    // viewer response
    //
    // this lets higher-level callers know of any potentially-relevant state
    // changes the viewer has detected
    struct Component3DViewerResponse final {
        OpenSim::Component const* hovertestResult = nullptr;
        bool isMousedOver = false;
        bool isLeftClicked = false;
        bool isRightClicked = false;
    };

    // a 3D viewer for a single OpenSim::Component or OpenSim::Model
    //
    // internally handles rendering, hit testing, etc. and exposes and API that lets
    // callers only have to handle `OpenSim::Model`s, `OpenSim::Component`s, etc.
    class Component3DViewer final {
    public:
        struct Impl;
    private:
        std::unique_ptr<Impl> m_Impl;

    public:
        Component3DViewer(Component3DViewerFlags = Component3DViewerFlags_Default);
        ~Component3DViewer() noexcept;

        bool isMousedOver() const noexcept;

        Component3DViewerResponse draw(
            char const* panelName,
            OpenSim::Component const&,
            OpenSim::ModelDisplayHints const&,
            SimTK::State const&,
            OpenSim::Component const* currentSelection,
            OpenSim::Component const* currentHover);

        Component3DViewerResponse draw(
            char const* panelName,
            OpenSim::Model const&,
            SimTK::State const&,
            OpenSim::Component const* currentSelection = nullptr,
            OpenSim::Component const* currentHover = nullptr);
    };
}

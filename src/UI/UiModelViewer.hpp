#pragma once

#include "src/OpenSimBindings/UiTypes.hpp"

namespace OpenSim {
    class Component;
}

namespace osc {
    // flags that toggle the viewer's behavior
    using UiModelViewerFlags = int;
    enum UiModelViewerFlags_ {

        // no flags: a basic-as-possible render
        RenderableSceneViewerFlags_None = 0,

        // draw a 2D XZ grid
        RenderableSceneViewerFlags_DrawXZGrid = 1 << 0,

        // draw a 2D XY grid
        RenderableSceneViewerFlags_DrawXYGrid = 1 << 1,

        // draw a 2D YZ grid
        RenderableSceneViewerFlags_DrawYZGrid = 1 << 2,

        // draw axis lines (the red/green lines on the floor showing axes)
        RenderableSceneViewerFlags_DrawAxisLines = 1 << 3,

        // draw AABBs (debugging)
        RenderableSceneViewerFlags_DrawAABBs = 1 << 4,

        // draw scene BVH (debugging)
        RenderableSceneViewerFlags_DrawBVH = 1 << 5,

        // draw alignment axes
        //
        // these are little red+green+blue demo axes in corner of the viewer that
        // show the user how the world axes align relative to the current view location
        RenderableSceneViewerFlags_DrawAlignmentAxes = 1 << 6,

        RenderableSceneViewerFlags_Default = RenderableSceneViewerFlags_None,
    };

    // viewer response
    //
    // this lets higher-level callers know of any potentially-relevant state
    // changes the viewer has detected
    struct UiModelViewerResponse final {
        OpenSim::Component const* hovertestResult = nullptr;
        bool isMousedOver = false;
        bool isLeftClicked = false;
        bool isRightClicked = false;
    };

    // a 3D viewer for a single OpenSim::Component or OpenSim::Model
    //
    // internally handles rendering, hit testing, etc. and exposes and API that lets
    // callers only have to handle `OpenSim::Model`s, `OpenSim::Component`s, etc.
    class UiModelViewer final {
    public:
        struct Impl;
    private:
        std::unique_ptr<Impl> m_Impl;

    public:
        UiModelViewer(UiModelViewerFlags = RenderableSceneViewerFlags_Default);
        ~UiModelViewer() noexcept;

        bool isMousedOver() const noexcept;

        UiModelViewerResponse draw(RenderableScene const&);
    };
}

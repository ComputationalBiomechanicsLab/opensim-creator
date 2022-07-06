#pragma once

#include <glm/vec3.hpp>

namespace OpenSim
{
    class Component;
}

namespace osc
{
    class VirtualConstModelStatePair;
}

namespace osc
{
    // flags that toggle the viewer's behavior
    using UiModelViewerFlags = int;
    enum UiModelViewerFlags_ {

        // no flags: a basic-as-possible render
        UiModelViewerFlags_None = 0,

        // draw the chequered floor
        UiModelViewerFlags_DrawFloor = 1 << 0,

        // draw a 2D XZ grid
        UiModelViewerFlags_DrawXZGrid = 1 << 1,

        // draw a 2D XY grid
        UiModelViewerFlags_DrawXYGrid = 1 << 2,

        // draw a 2D YZ grid
        UiModelViewerFlags_DrawYZGrid = 1 << 3,

        // draw axis lines (the red/green lines on the floor showing axes)
        UiModelViewerFlags_DrawAxisLines = 1 << 4,

        // draw AABBs (debugging)
        UiModelViewerFlags_DrawAABBs = 1 << 5,

        // draw scene BVH (debugging)
        UiModelViewerFlags_DrawBVH = 1 << 6,

        // draw alignment axes
        //
        // these are little red+green+blue demo axes in corner of the viewer that
        // show the user how the world axes align relative to the current view location
        UiModelViewerFlags_DrawAlignmentAxes = 1 << 7,

        UiModelViewerFlags_Default = UiModelViewerFlags_DrawFloor | UiModelViewerFlags_DrawAlignmentAxes,
    };

    // viewer response
    //
    // this lets higher-level callers know of any potentially-relevant state
    // changes the viewer has detected
    struct UiModelViewerResponse final {
        OpenSim::Component const* hovertestResult = nullptr;
        bool isMousedOver = false;
        glm::vec3 mouse3DLocation = {0.0f, 0.0f, 0.0f};
        bool isLeftClicked = false;
        bool isRightClicked = false;
    };

    // a 3D viewer for a single OpenSim::Component or OpenSim::Model
    //
    // internally handles rendering, hit testing, etc. and exposes and API that lets
    // callers only have to handle `OpenSim::Model`s, `OpenSim::Component`s, etc.
    class UiModelViewer final {
    public:
        UiModelViewer(UiModelViewerFlags = UiModelViewerFlags_Default);
        UiModelViewer(UiModelViewer const&) = delete;
        UiModelViewer(UiModelViewer&&) noexcept;
        UiModelViewer& operator=(UiModelViewer const&) = delete;
        UiModelViewer& operator=(UiModelViewer&&) noexcept;
        ~UiModelViewer() noexcept;

        bool isMousedOver() const;
        void requestAutoFocus();
        UiModelViewerResponse draw(VirtualConstModelStatePair const&);

    private:
        class Impl;
        Impl* m_Impl;
    };
}

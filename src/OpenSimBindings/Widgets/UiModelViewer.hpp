#pragma once

#include "src/OpenSimBindings/Widgets/UiModelViewerResponse.hpp"

#include <memory>

namespace osc { class VirtualConstModelStatePair; }

namespace osc
{

    // a 3D viewer for a single OpenSim::Component or OpenSim::Model
    //
    // internally handles rendering, hit testing, etc. and exposes and API that lets
    // callers only have to handle `OpenSim::Model`s, `OpenSim::Component`s, etc.
    class UiModelViewer final {
    public:
        UiModelViewer();
        UiModelViewer(UiModelViewer const&) = delete;
        UiModelViewer(UiModelViewer&&) noexcept;
        UiModelViewer& operator=(UiModelViewer const&) = delete;
        UiModelViewer& operator=(UiModelViewer&&) noexcept;
        ~UiModelViewer() noexcept;

        bool isMousedOver() const;
        bool isLeftClicked() const;
        bool isRightClicked() const;
        UiModelViewerResponse draw(VirtualConstModelStatePair const&);

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

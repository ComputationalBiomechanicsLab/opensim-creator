#pragma once

#include "src/Graphics/SceneCollision.hpp"
#include "src/Maths/Rect.hpp"

#include <memory>
#include <optional>

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
        std::optional<SceneCollision> draw(VirtualConstModelStatePair const&);
        std::optional<osc::Rect> getScreenRect() const;

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

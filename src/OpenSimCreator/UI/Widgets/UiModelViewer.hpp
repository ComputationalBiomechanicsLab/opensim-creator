#pragma once

#include <oscar/Maths/Rect.hpp>
#include <oscar/Scene/SceneCollision.hpp>

#include <memory>
#include <optional>
#include <string_view>

namespace osc { class VirtualConstModelStatePair; }

namespace osc
{

    // a 3D viewer for a single OpenSim::Component or OpenSim::Model
    //
    // internally handles rendering, hit testing, etc. and exposes and API that lets
    // callers only have to handle `OpenSim::Model`s, `OpenSim::Component`s, etc.
    class UiModelViewer final {
    public:
        UiModelViewer(std::string_view parentPanelName_);
        UiModelViewer(UiModelViewer const&) = delete;
        UiModelViewer(UiModelViewer&&) noexcept;
        UiModelViewer& operator=(UiModelViewer const&) = delete;
        UiModelViewer& operator=(UiModelViewer&&) noexcept;
        ~UiModelViewer() noexcept;

        bool isMousedOver() const;
        bool isLeftClicked() const;
        bool isRightClicked() const;
        std::optional<SceneCollision> onDraw(VirtualConstModelStatePair const&);
        std::optional<Rect> getScreenRect() const;

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

#pragma once

#include <oscar/Maths/Rect.h>
#include <oscar/Scene/SceneCollision.h>

#include <memory>
#include <optional>
#include <string_view>

namespace osc { class IConstModelStatePair; }

namespace osc
{

    // readonly 3D viewer for a single OpenSim::Model
    //
    // internally handles rendering, hit testing, etc. and exposes and API that lets
    // callers only have to handle `OpenSim::Model`s, `OpenSim::Component`s, etc.
    class Readonly3DModelViewer final {
    public:
        explicit Readonly3DModelViewer(std::string_view parentPanelName_);
        Readonly3DModelViewer(Readonly3DModelViewer const&) = delete;
        Readonly3DModelViewer(Readonly3DModelViewer&&) noexcept;
        Readonly3DModelViewer& operator=(Readonly3DModelViewer const&) = delete;
        Readonly3DModelViewer& operator=(Readonly3DModelViewer&&) noexcept;
        ~Readonly3DModelViewer() noexcept;

        bool isMousedOver() const;
        bool isLeftClicked() const;
        bool isRightClicked() const;
        std::optional<SceneCollision> onDraw(IConstModelStatePair const&);
        std::optional<Rect> getScreenRect() const;

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

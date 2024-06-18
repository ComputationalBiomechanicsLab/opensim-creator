#pragma once

#include <oscar/Graphics/Scene/SceneCollision.h>
#include <oscar/Shims/Cpp23/utility.h>
#include <oscar/Maths/Rect.h>

#include <memory>
#include <optional>
#include <string_view>

namespace osc { class IConstModelStatePair; }
namespace osc { struct PolarPerspectiveCamera; }

namespace osc
{
    enum class Readonly3DModelViewerFlags {
        None           = 0,
        NoSceneHittest = 1<<0,
        NUM_OPTIONS
    };

    constexpr bool operator&(Readonly3DModelViewerFlags lhs, Readonly3DModelViewerFlags rhs)
    {
        return (cpp23::to_underlying(lhs) & cpp23::to_underlying(rhs)) != 0;
    }

    // readonly 3D viewer for a single OpenSim::Model
    //
    // internally handles rendering, hit testing, etc. and exposes and API that lets
    // callers only have to handle `OpenSim::Model`s, `OpenSim::Component`s, etc.
    class Readonly3DModelViewer final {
    public:
        Readonly3DModelViewer(
            std::string_view parentPanelName_,
            Readonly3DModelViewerFlags = Readonly3DModelViewerFlags::None
        );
        Readonly3DModelViewer(const Readonly3DModelViewer&) = delete;
        Readonly3DModelViewer(Readonly3DModelViewer&&) noexcept;
        Readonly3DModelViewer& operator=(const Readonly3DModelViewer&) = delete;
        Readonly3DModelViewer& operator=(Readonly3DModelViewer&&) noexcept;
        ~Readonly3DModelViewer() noexcept;

        bool isMousedOver() const;
        bool isLeftClicked() const;
        bool isRightClicked() const;
        std::optional<SceneCollision> onDraw(const IConstModelStatePair&);
        std::optional<Rect> getScreenRect() const;
        const PolarPerspectiveCamera& getCamera() const;
        void setCamera(const PolarPerspectiveCamera&);

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

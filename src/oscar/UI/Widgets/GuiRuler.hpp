#pragma once

#include <oscar/Maths/Vec3.hpp>
#include <oscar/Scene/SceneCollision.hpp>

#include <optional>
#include <string>

namespace osc { struct PolarPerspectiveCamera; }
namespace osc { struct Rect; }

namespace osc
{
    class GuiRuler final {
    public:
        void onDraw(
            PolarPerspectiveCamera const&,
            Rect const& renderRect,
            std::optional<SceneCollision>
        );
        void startMeasuring();
        void stopMeasuring();
        void toggleMeasuring();
        bool isMeasuring() const;

    private:
        enum class State { Inactive, WaitingForFirstPoint, WaitingForSecondPoint };
        State m_State = State::Inactive;
        Vec3 m_StartWorldPos = {};
    };
}

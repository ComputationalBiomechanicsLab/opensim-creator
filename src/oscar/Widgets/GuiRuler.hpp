#pragma once

#include "oscar/Graphics/SceneCollision.hpp"

#include <glm/vec3.hpp>

#include <optional>
#include <string>

namespace osc { struct PolarPerspectiveCamera; }
namespace osc { struct Rect; }

namespace osc
{
    class GuiRuler final {
    public:
        void draw(
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
        glm::vec3 m_StartWorldPos = {0.0f, 0.0f, 0.0f};
    };
}

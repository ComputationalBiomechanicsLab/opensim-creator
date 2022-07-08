#pragma once

#include <glm/vec3.hpp>

#include <optional>
#include <string>

namespace osc
{
    class PolarPerspectiveCamera;
    struct Rect;
}

namespace osc
{
    class GuiRulerMouseHit final {
    public:
        std::string Name;
        glm::vec3 WorldPos;

        GuiRulerMouseHit(std::string name, glm::vec3 const& worldPos);
    };

    class GuiRuler final {
    public:
        void draw(PolarPerspectiveCamera const&, Rect const& renderRect, std::optional<GuiRulerMouseHit>);
        void startMeasuring();
        void stopMeasuring();
        bool isMeasuring() const;

    private:
        enum class State { Inactive, WaitingForFirstPoint, WaitingForSecondPoint };
        State m_State = State::Inactive;
        glm::vec3 m_StartWorldPos = {0.0f, 0.0f, 0.0f};
    };
}
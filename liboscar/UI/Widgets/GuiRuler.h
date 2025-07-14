#pragma once

#include <liboscar/Graphics/Scene/SceneCollision.h>
#include <liboscar/Maths/Vec3.h>

#include <optional>

namespace osc { class Rect; }
namespace osc { struct PolarPerspectiveCamera; }

namespace osc
{
    class GuiRuler final {
    public:
        void on_draw(
            const PolarPerspectiveCamera&,
            const Rect& render_rect,
            std::optional<SceneCollision>
        );
        void start_measuring();
        void stop_measuring();
        void toggle_measuring();
        bool is_measuring() const;

    private:
        enum class State { Inactive, WaitingForFirstPoint, WaitingForSecondPoint };
        State state_ = State::Inactive;
        Vec3 start_world_pos_ = {};
    };
}

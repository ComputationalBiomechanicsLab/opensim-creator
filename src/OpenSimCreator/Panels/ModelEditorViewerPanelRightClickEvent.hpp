#pragma once

#include <oscar/Maths/Rect.hpp>

#include <glm/vec3.hpp>

#include <optional>
#include <string>
#include <utility>

namespace osc
{
    struct ModelEditorViewerPanelRightClickEvent final {
        ModelEditorViewerPanelRightClickEvent(
            Rect const& viewportScreenRect_,
            std::string componentAbsPathOrEmpty_,
            std::optional<glm::vec3> maybeClickPositionInGround_) :

            viewportScreenRect{viewportScreenRect_},
            componentAbsPathOrEmpty{std::move(componentAbsPathOrEmpty_)},
            maybeClickPositionInGround{maybeClickPositionInGround_}
        {
        }

        Rect viewportScreenRect;
        std::string componentAbsPathOrEmpty;
        std::optional<glm::vec3> maybeClickPositionInGround;
    };
}
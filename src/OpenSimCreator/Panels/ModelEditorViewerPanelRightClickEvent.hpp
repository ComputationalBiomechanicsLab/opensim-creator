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
            std::string sourcePanelName_,
            Rect const& viewportScreenRect_,
            std::string componentAbsPathOrEmpty_,
            std::optional<glm::vec3> maybeClickPositionInGround_) :

            sourcePanelName{std::move(sourcePanelName_)},
            viewportScreenRect{viewportScreenRect_},
            componentAbsPathOrEmpty{std::move(componentAbsPathOrEmpty_)},
            maybeClickPositionInGround{maybeClickPositionInGround_}
        {
        }

        std::string sourcePanelName;
        Rect viewportScreenRect;
        std::string componentAbsPathOrEmpty;
        std::optional<glm::vec3> maybeClickPositionInGround;
    };
}
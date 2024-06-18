#pragma once

#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Vec3.h>

#include <optional>
#include <string>
#include <utility>

namespace osc
{
    struct ModelEditorViewerPanelRightClickEvent final {

        ModelEditorViewerPanelRightClickEvent(
            std::string sourcePanelName_,
            const Rect& viewportScreenRect_,
            std::string componentAbsPathOrEmpty_,
            std::optional<Vec3> maybeClickPositionInGround_) :

            sourcePanelName{std::move(sourcePanelName_)},
            viewportScreenRect{viewportScreenRect_},
            componentAbsPathOrEmpty{std::move(componentAbsPathOrEmpty_)},
            maybeClickPositionInGround{maybeClickPositionInGround_}
        {
        }

        std::string sourcePanelName;
        Rect viewportScreenRect;
        std::string componentAbsPathOrEmpty;
        std::optional<Vec3> maybeClickPositionInGround;
    };
}

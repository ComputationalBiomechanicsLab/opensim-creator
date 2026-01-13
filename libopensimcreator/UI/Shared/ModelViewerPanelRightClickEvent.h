#pragma once

#include <liboscar/maths/Rect.h>
#include <liboscar/maths/Vector3.h>

#include <optional>
#include <string>
#include <utility>

namespace osc
{
    struct ModelViewerPanelRightClickEvent final {

        ModelViewerPanelRightClickEvent(
            std::string sourcePanelName_,
            const Rect& viewportScreenRect_,
            std::string componentAbsPathOrEmpty_,
            std::optional<Vector3> maybeClickPositionInGround_) :

            sourcePanelName{std::move(sourcePanelName_)},
            viewportScreenRect{viewportScreenRect_},
            componentAbsPathOrEmpty{std::move(componentAbsPathOrEmpty_)},
            maybeClickPositionInGround{maybeClickPositionInGround_}
        {}

        std::string sourcePanelName;
        Rect viewportScreenRect;
        std::string componentAbsPathOrEmpty;
        std::optional<Vector3> maybeClickPositionInGround;
    };
}

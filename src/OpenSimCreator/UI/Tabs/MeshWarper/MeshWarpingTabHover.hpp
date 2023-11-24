#pragma once

#include <OpenSimCreator/Documents/MeshWarp/TPSDocumentElementID.hpp>

#include <oscar/Maths/Vec3.hpp>

#include <optional>

namespace osc
{
    // a mouse hovertest result
    struct MeshWarpingTabHover final {

        explicit MeshWarpingTabHover(Vec3 const& worldspaceLocation_) :
            worldspaceLocation{worldspaceLocation_}
        {
        }

        MeshWarpingTabHover(
            TPSDocumentElementID sceneElementID_,
            Vec3 const& worldspaceLocation_) :

            maybeSceneElementID{std::move(sceneElementID_)},
            worldspaceLocation{worldspaceLocation_}
        {
        }

        std::optional<TPSDocumentElementID> maybeSceneElementID = std::nullopt;
        Vec3 worldspaceLocation;
    };
}

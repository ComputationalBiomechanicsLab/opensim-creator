#pragma once

#include <oscar/Maths/Vec3.hpp>

#include <optional>
#include <string>
#include <utility>

namespace osc
{
    // a landmark pair in the TPS document (might be midway through definition)
    struct TPSDocumentLandmarkPair final {

        explicit TPSDocumentLandmarkPair(std::string id_) :
            id{std::move(id_)}
        {
        }

        std::string id;
        std::optional<Vec3> maybeSourceLocation;
        std::optional<Vec3> maybeDestinationLocation;
    };
}

#pragma once

#include <oscar/Maths/Vec3.hpp>
#include <oscar/Utils/StringName.hpp>

#include <optional>
#include <utility>

namespace osc
{
    // a landmark pair in the TPS document (might be midway through definition)
    struct TPSDocumentLandmarkPair final {

        explicit TPSDocumentLandmarkPair(StringName id_) :
            id{std::move(id_)}
        {
        }

        StringName id;
        std::optional<Vec3> maybeSourceLocation;
        std::optional<Vec3> maybeDestinationLocation;
    };
}

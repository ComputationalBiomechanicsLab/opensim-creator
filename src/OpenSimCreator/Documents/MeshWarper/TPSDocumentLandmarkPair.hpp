#pragma once

#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentElement.hpp>

#include <oscar/Maths/Vec3.hpp>
#include <oscar/Utils/StringName.hpp>
#include <oscar/Utils/UID.hpp>

#include <optional>
#include <utility>

namespace osc
{
    // a landmark pair in the TPS document (might be midway through definition)
    struct TPSDocumentLandmarkPair final : public TPSDocumentElement {

        explicit TPSDocumentLandmarkPair(StringName name_) :
            name{std::move(name_)}
        {
        }

        UID uid;
        StringName name;
        std::optional<Vec3> maybeSourceLocation;
        std::optional<Vec3> maybeDestinationLocation;

    private:
        CStringView implGetName() const final
        {
            return name;
        }
    };
}

#pragma once

#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentElement.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentElementID.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentElementType.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentInputIdentifier.hpp>

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

        TPSDocumentElementID sourceID() const
        {
            return TPSDocumentElementID{uid, TPSDocumentElementType::Landmark, TPSDocumentInputIdentifier::Source};
        }

        TPSDocumentElementID destinationID() const
        {
            return TPSDocumentElementID{uid, TPSDocumentElementType::Landmark, TPSDocumentInputIdentifier::Destination};
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

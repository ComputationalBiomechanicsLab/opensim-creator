#pragma once

#include <libopensimcreator/Documents/MeshWarper/TPSDocumentElement.h>
#include <libopensimcreator/Documents/MeshWarper/TPSDocumentElementID.h>
#include <libopensimcreator/Documents/MeshWarper/TPSDocumentElementType.h>
#include <libopensimcreator/Documents/MeshWarper/TPSDocumentInputIdentifier.h>

#include <liboscar/Maths/Vec3.h>
#include <liboscar/Utils/StringName.h>
#include <liboscar/Utils/UID.h>

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

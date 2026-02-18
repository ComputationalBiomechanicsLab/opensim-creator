#pragma once

#include <libopensimcreator/documents/mesh_warper/tps_document_element.h>
#include <libopensimcreator/documents/mesh_warper/tps_document_element_id.h>
#include <libopensimcreator/documents/mesh_warper/tps_document_element_type.h>
#include <libopensimcreator/documents/mesh_warper/tps_document_input_identifier.h>

#include <liboscar/maths/vector3.h>
#include <liboscar/utilities/string_name.h>
#include <liboscar/utilities/uid.h>

#include <concepts>
#include <optional>
#include <utility>

namespace osc
{
    // a landmark pair in the TPS document (might be midway through definition)
    struct TPSDocumentLandmarkPair final : public TPSDocumentElement {

        explicit TPSDocumentLandmarkPair(StringName name_) :
            name{std::move(name_)}
        {}

        template<typename StringLike>
        requires std::constructible_from<StringName, StringLike&&>
        explicit TPSDocumentLandmarkPair(
            StringLike&& name_,
            std::optional<Vector3> maybeSourceLocation_,
            std::optional<Vector3> maybeDestinationLocation_) :

            name{std::forward<StringLike>(name_)},
            maybeSourceLocation{std::move(maybeSourceLocation_)},
            maybeDestinationLocation{std::move(maybeDestinationLocation_)}
        {}

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
        std::optional<Vector3> maybeSourceLocation;
        std::optional<Vector3> maybeDestinationLocation;

    private:
        CStringView implGetName() const final
        {
            return name;
        }
    };
}

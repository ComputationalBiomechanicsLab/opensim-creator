#pragma once

#include <libopensimcreator/documents/mesh_warper/mw_document_element.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_element_id.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_element_type.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_input_identifier.h>

#include <liboscar/maths/vector.h>
#include <liboscar/utilities/string_name.h>
#include <liboscar/utilities/uid.h>

#include <concepts>
#include <optional>
#include <utility>

namespace osc
{
    /// A landmark pair in the TPS document, which might be midway through definition.
    struct MwDocumentLandmarkPair final : public MwDocumentElement {

        explicit MwDocumentLandmarkPair(StringName name_) :
            name{std::move(name_)}
        {}

        template<typename StringLike>
        requires std::constructible_from<StringName, StringLike&&>
        explicit MwDocumentLandmarkPair(
            StringLike&& name_,
            std::optional<Vector3> maybeSourceLocation_,
            std::optional<Vector3> maybeDestinationLocation_) :

            name{std::forward<StringLike>(name_)},
            maybeSourceLocation{std::move(maybeSourceLocation_)},
            maybeDestinationLocation{std::move(maybeDestinationLocation_)}
        {}

        MwDocumentElementID sourceID() const
        {
            return MwDocumentElementID{uid, MwDocumentElementType::Landmark, MiDocumentInputIdentifier::Source};
        }

        MwDocumentElementID destinationID() const
        {
            return MwDocumentElementID{uid, MwDocumentElementType::Landmark, MiDocumentInputIdentifier::Destination};
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

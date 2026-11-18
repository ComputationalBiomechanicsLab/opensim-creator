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

        explicit MwDocumentLandmarkPair(StringName name) :
            name{std::move(name)}
        {}

        template<typename StringLike>
        requires std::constructible_from<StringName, StringLike&&>
        explicit MwDocumentLandmarkPair(
            StringLike&& name,
            std::optional<Vector3> maybe_source_location,
            std::optional<Vector3> maybe_destination_location) :

            name{std::forward<StringLike>(name)},
            maybe_source_location{maybe_source_location},
            maybe_destination_location{maybe_destination_location}
        {}

        MwDocumentElementID source_id() const
        {
            return MwDocumentElementID{
                uid, MwDocumentElementType::Landmark, MiDocumentInputIdentifier::Source};
        }

        MwDocumentElementID destination_id() const
        {
            return MwDocumentElementID{
                uid, MwDocumentElementType::Landmark, MiDocumentInputIdentifier::Destination};
        }

        UID uid;
        StringName name;
        std::optional<Vector3> maybe_source_location;
        std::optional<Vector3> maybe_destination_location;

    private:
        CStringView impl_get_name() const override { return name; }
    };
}

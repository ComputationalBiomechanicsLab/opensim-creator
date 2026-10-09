#pragma once

#include <libopensimcreator/documents/mesh_warper/mw_document_element.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_element_id.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_element_type.h>

#include <liboscar/maths/vector.h>
#include <liboscar/utilities/string_name.h>
#include <liboscar/utilities/uid.h>

namespace osc
{
    /// Represents a "non-participating landmark" in a `MwDocument`.
    ///
    /// Non-participating landmarks are effectively datapoints that should be warped
    /// by the TPS warping equation, but shouldn't contribute to computing the
    /// coefficients of the warping equation.
    struct MwDocumentNonParticipatingLandmark final : public MwDocumentElement {
    public:
        MwDocumentNonParticipatingLandmark(
            const StringName& name_,
            const Vector3& location_) :

            name{name_},
            location{location_}
        {
        }

        MwDocumentElementID getID() const
        {
            return MwDocumentElementID{uid, MwDocumentElementType::NonParticipatingLandmark};
        }

        UID uid;
        StringName name;
        Vector3 location;

    private:
        CStringView implGetName() const final
        {
            return name;
        }
    };
}

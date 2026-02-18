#pragma once

#include <libopensimcreator/documents/mesh_warper/tps_document_element.h>
#include <libopensimcreator/documents/mesh_warper/tps_document_element_id.h>
#include <libopensimcreator/documents/mesh_warper/tps_document_element_type.h>

#include <liboscar/maths/vector3.h>
#include <liboscar/utilities/string_name.h>
#include <liboscar/utilities/uid.h>

namespace osc
{
    struct TPSDocumentNonParticipatingLandmark final : public TPSDocumentElement {
    public:
        TPSDocumentNonParticipatingLandmark(
            const StringName& name_,
            const Vector3& location_) :

            name{name_},
            location{location_}
        {
        }

        TPSDocumentElementID getID() const
        {
            return TPSDocumentElementID{uid, TPSDocumentElementType::NonParticipatingLandmark};
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

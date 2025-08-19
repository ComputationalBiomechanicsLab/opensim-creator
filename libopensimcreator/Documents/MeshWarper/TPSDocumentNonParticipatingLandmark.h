#pragma once

#include <libopensimcreator/Documents/MeshWarper/TPSDocumentElement.h>
#include <libopensimcreator/Documents/MeshWarper/TPSDocumentElementID.h>
#include <libopensimcreator/Documents/MeshWarper/TPSDocumentElementType.h>

#include <liboscar/Maths/Vector3.h>
#include <liboscar/Utils/StringName.h>
#include <liboscar/Utils/UID.h>

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

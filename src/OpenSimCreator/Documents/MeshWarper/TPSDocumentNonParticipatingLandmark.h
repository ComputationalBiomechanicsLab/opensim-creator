#pragma once

#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentElement.h>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentElementID.h>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentElementType.h>

#include <oscar/Maths/Vec3.h>
#include <oscar/Utils/StringName.h>
#include <oscar/Utils/UID.h>

namespace osc
{
    struct TPSDocumentNonParticipatingLandmark final : public TPSDocumentElement {
    public:
        TPSDocumentNonParticipatingLandmark(
            const StringName& name_,
            const Vec3& location_) :

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
        Vec3 location;

    private:
        CStringView implGetName() const final
        {
            return name;
        }
    };
}

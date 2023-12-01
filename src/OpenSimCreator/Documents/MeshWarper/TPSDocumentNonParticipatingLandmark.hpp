#pragma once

#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentElement.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentElementType.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentElementID.hpp>

#include <oscar/Maths/Vec3.hpp>
#include <oscar/Utils/StringName.hpp>
#include <oscar/Utils/UID.hpp>

namespace osc
{
    struct TPSDocumentNonParticipatingLandmark final : public TPSDocumentElement {
    public:
        TPSDocumentNonParticipatingLandmark(
            StringName const& name_,
            Vec3 const& location_) :

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

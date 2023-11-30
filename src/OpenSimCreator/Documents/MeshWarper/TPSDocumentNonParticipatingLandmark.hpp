#pragma once

#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentElement.hpp>

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

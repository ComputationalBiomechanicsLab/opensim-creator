#pragma once

#include <oscar/Maths/Vec3.hpp>
#include <oscar/Utils/StringName.hpp>

namespace osc
{
    struct TPSDocumentNonParticipatingLandmark final {
        TPSDocumentNonParticipatingLandmark(
            StringName const& id_,
            Vec3 const& location_) :

            id{id_},
            location{location_}
        {
        }

        StringName id;
        Vec3 location;
    };
}

#pragma once

#include <OpenSimCreator/Documents/MeshImporter/MIIDs.h>

#include <oscar/Maths/Vec3.h>
#include <oscar/Utils/UID.h>

namespace osc::mi
{
    class MeshImporterHover final {
    public:
        MeshImporterHover() :
            ID{MIIDs::Empty()},
            Pos{}
        {
        }

        MeshImporterHover(UID id_, Vec3 pos_) :
            ID{id_},
            Pos{pos_}
        {
        }

        explicit operator bool () const
        {
            return ID != MIIDs::Empty();
        }

        void reset()
        {
            *this = MeshImporterHover{};
        }

        UID ID;
        Vec3 Pos;
    };
}

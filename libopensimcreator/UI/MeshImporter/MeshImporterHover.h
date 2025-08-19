#pragma once

#include <libopensimcreator/Documents/MeshImporter/MIIDs.h>

#include <liboscar/Maths/Vector3.h>
#include <liboscar/Utils/UID.h>

namespace osc::mi
{
    class MeshImporterHover final {
    public:
        MeshImporterHover() :
            ID{MIIDs::Empty()},
            Pos{}
        {
        }

        MeshImporterHover(UID id_, Vector3 pos_) :
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
        Vector3 Pos;
    };
}

#pragma once

#include <libopensimcreator/documents/mesh_importer/mi_ids.h>

#include <liboscar/maths/vector3.h>
#include <liboscar/utilities/uid.h>

namespace osc
{
    class MeshImporterHover final {
    public:
        MeshImporterHover() :
            ID{MiIDs::Empty()},
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
            return ID != MiIDs::Empty();
        }

        void reset()
        {
            *this = MeshImporterHover{};
        }

        UID ID;
        Vector3 Pos;
    };
}

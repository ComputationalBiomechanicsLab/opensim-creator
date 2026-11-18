#pragma once

#include <libopensimcreator/documents/mesh_importer/mi_ids.h>

#include <liboscar/maths/vector.h>
#include <liboscar/utilities/uid.h>

namespace osc
{
    class MeshImporterHover final {
    public:
        MeshImporterHover() = default;

        MeshImporterHover(UID id, Vector3 pos) :
            id{id},
            pos{pos}
        {}

        explicit operator bool() const { return id != MiIDs::empty(); }

        void reset() { *this = MeshImporterHover{}; }

        UID id = MiIDs::empty();
        Vector3 pos;
    };
}

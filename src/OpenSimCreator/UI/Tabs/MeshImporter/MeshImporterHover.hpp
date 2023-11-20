#pragma once

#include <OpenSimCreator/ModelGraph/ModelGraphIDs.hpp>

#include <oscar/Maths/Vec3.hpp>
#include <oscar/Utils/UID.hpp>

namespace osc
{
    class MeshImporterHover final {
    public:
        MeshImporterHover() :
            ID{ModelGraphIDs::Empty()},
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
            return ID != ModelGraphIDs::Empty();
        }

        void reset()
        {
            *this = MeshImporterHover{};
        }

        UID ID;
        Vec3 Pos;
    };
}

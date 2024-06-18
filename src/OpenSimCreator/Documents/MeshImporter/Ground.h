#pragma once

#include <OpenSimCreator/Documents/MeshImporter/MIObjectCRTP.h>
#include <OpenSimCreator/Documents/MeshImporter/MIObjectFlags.h>

#include <oscar/Maths/AABB.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <iosfwd>

namespace osc::mi
{
    // "Ground" of the scene (i.e. origin)
    class Ground final : public MIObjectCRTP<Ground> {
    private:
        friend class MIObjectCRTP<Ground>;
        static MIClass CreateClass();

        MIObjectFlags implGetFlags() const final
        {
            return MIObjectFlags::None;
        }

        UID implGetID() const final;

        std::ostream& implWriteToStream(std::ostream&) const final;

        CStringView implGetLabel() const final;

        Transform implGetXform(const IObjectFinder&) const final
        {
            return identity<Transform>();
        }

        AABB implCalcBounds(const IObjectFinder&) const final
        {
            return AABB{};
        }
    };
}

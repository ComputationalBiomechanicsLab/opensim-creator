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

        Transform implGetXform(IObjectFinder const&) const final
        {
            return Identity<Transform>();
        }

        AABB implCalcBounds(IObjectFinder const&) const final
        {
            return AABB{};
        }
    };
}

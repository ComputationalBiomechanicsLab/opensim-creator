#pragma once

#include <OpenSimCreator/Documents/MeshImporter/MIObjectCRTP.hpp>
#include <OpenSimCreator/Documents/MeshImporter/MIObjectFlags.hpp>

#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>

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

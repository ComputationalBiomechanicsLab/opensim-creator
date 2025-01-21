#pragma once

#include <libopensimcreator/Documents/MeshImporter/MIObjectCRTP.h>
#include <libopensimcreator/Documents/MeshImporter/MIObjectFlags.h>

#include <liboscar/Maths/AABB.h>
#include <liboscar/Maths/Transform.h>
#include <liboscar/Utils/CStringView.h>
#include <liboscar/Utils/UID.h>

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

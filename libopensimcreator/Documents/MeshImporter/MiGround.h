#pragma once

#include <libopensimcreator/Documents/MeshImporter/MiObjectCRTP.h>
#include <libopensimcreator/Documents/MeshImporter/MiObjectFlags.h>

#include <liboscar/maths/aabb.h>
#include <liboscar/maths/transform.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/uid.h>

#include <iosfwd>

namespace osc
{
    // "Ground" of the scene (i.e. origin)
    class MiGround final : public MiObjectCRTP<MiGround> {
    private:
        friend class MiObjectCRTP<MiGround>;
        static MiClass CreateClass();

        MiObjectFlags implGetFlags() const final
        {
            return MiObjectFlags::None;
        }

        UID implGetID() const final;

        std::ostream& implWriteToStream(std::ostream&) const final;

        CStringView implGetLabel() const final;

        Transform implGetXform(const MiObjectFinder&) const final
        {
            return identity<Transform>();
        }

        std::optional<AABB> implCalcBounds(const MiObjectFinder&) const final
        {
            return std::nullopt;
        }
    };
}

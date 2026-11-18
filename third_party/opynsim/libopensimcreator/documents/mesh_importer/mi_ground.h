#pragma once

#include <libopensimcreator/documents/mesh_importer/mi_object_crtp.h>
#include <libopensimcreator/documents/mesh_importer/mi_object_flags.h>

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
        static MiClass create_class();

        MiObjectFlags impl_get_flags() const override { return MiObjectFlags::None; }

        UID impl_get_id() const override;

        std::ostream& impl_write_to_stream(std::ostream&) const override;

        CStringView impl_get_label() const override;

        Transform impl_get_xform(const MiObjectFinder&) const override
        {
            return identity<Transform>();
        }

        std::optional<AABB> impl_calc_bounds(const MiObjectFinder&) const override
        {
            return std::nullopt;
        }
    };
}

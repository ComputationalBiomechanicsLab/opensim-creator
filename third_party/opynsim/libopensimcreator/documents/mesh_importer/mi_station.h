#pragma once

#include <libopensimcreator/documents/mesh_importer/mi_crossref_descriptor.h>
#include <libopensimcreator/documents/mesh_importer/mi_object_crtp.h>

#include <liboscar/maths/aabb.h>
#include <liboscar/maths/aabb_functions.h>
#include <liboscar/maths/transform.h>
#include <liboscar/maths/vector.h>
#include <liboscar/utilities/uid.h>

#include <iosfwd>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace osc
{
    // a station (point of interest)
    class MiStation final : public MiObjectCRTP<MiStation> {
    public:
        MiStation(
            UID id,
            UID attachment,  // can be MiIDs::ground()
            const Vector3& position,
            const std::string& name);

        MiStation(
            UID attachment,  // can be MiIDs::ground()
            const Vector3& position,
            const std::string& name);

        UID get_parent_id() const { return attachment_; }

        Transform get_x_form() const { return Transform{.translation = position_}; }

    private:
        friend class MiObjectCRTP<MiStation>;
        static MiClass create_class();

        std::vector<MiCrossrefDescriptor> impl_get_cross_references() const override;

        void impl_set_cross_reference_connectee_id(int i, UID id) override
        {
            if (i != 0) {
                throw std::runtime_error{"invalid index accessed for cross reference"};
            }
            attachment_ = id;
        }

        MiObjectFlags impl_get_flags() const override
        {
            return MiObjectFlags::CanChangeLabel |     //
                   MiObjectFlags::CanChangePosition |  //
                   MiObjectFlags::CanDelete |          //
                   MiObjectFlags::CanSelect;
        }

        UID impl_get_id() const override { return id_; }

        std::ostream& impl_write_to_stream(std::ostream&) const override;

        CStringView impl_get_label() const override { return name_; }

        void impl_set_label(std::string_view) override;

        Transform impl_get_xform(const MiObjectFinder&) const override { return get_x_form(); }

        void impl_set_xform(const MiObjectFinder&, const Transform& t) override
        {
            position_ = t.translation;
        }

        std::optional<AABB> impl_calc_bounds(const MiObjectFinder&) const override
        {
            return bounding_aabb_of(position_);
        }

        UID id_;
        UID attachment_;  // can be MiIDs::ground()
        Vector3 position_;
        std::string name_;
    };
}

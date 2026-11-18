#pragma once

#include <libopensimcreator/documents/mesh_importer/mi_crossref_descriptor.h>
#include <libopensimcreator/documents/mesh_importer/mi_object_crtp.h>
#include <libopensimcreator/documents/mesh_importer/mi_object_flags.h>

#include <liboscar/maths/aabb.h>
#include <liboscar/maths/aabb_functions.h>
#include <liboscar/maths/transform.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/uid.h>

#include <cstddef>
#include <iosfwd>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace osc
{
    // a joint, as defined by the mesh importer
    class MiJoint final : public MiObjectCRTP<MiJoint> {
    public:
        MiJoint(
            UID id,
            std::string joint_type_name,
            const std::string& user_assigned_name,  // can be empty
            UID parent,
            UID child,
            const Transform& xform);

        CStringView get_specific_type_name() const { return joint_type_name_; }

        void set_specific_type_name(std::string_view new_name) { joint_type_name_ = new_name; }

        UID get_parent_id() const { return parent_; }

        UID get_child_id() const { return child_; }

        CStringView get_user_assigned_name() const { return user_assigned_name_; }

        Transform get_x_form() const { return xform_; }

    private:
        friend class MiObjectCRTP<MiJoint>;
        static MiClass create_class();

        std::vector<MiCrossrefDescriptor> impl_get_cross_references() const override;

        void impl_set_cross_reference_connectee_id(int i, UID id) override
        {
            switch (i) {
            case 0: parent_ = id; break;
            case 1: child_ = id; break;
            default: throw std::runtime_error{"invalid index accessed for cross reference"};
            }
        }

        MiObjectFlags impl_get_flags() const override
        {
            return MiObjectFlags::CanChangeLabel |     //
                   MiObjectFlags::CanChangePosition |  //
                   MiObjectFlags::CanChangeRotation |  //
                   MiObjectFlags::CanDelete |          //
                   MiObjectFlags::CanSelect;
        }

        UID impl_get_id() const override { return id_; }

        std::ostream& impl_write_to_stream(std::ostream&) const override;

        CStringView impl_get_label() const override
        {
            return user_assigned_name_.empty() ? get_specific_type_name() : user_assigned_name_;
        }

        void impl_set_label(std::string_view) override;

        Transform impl_get_xform(const MiObjectFinder&) const override { return get_x_form(); }

        void impl_set_xform(const MiObjectFinder&, const Transform& t) override
        {
            xform_ = t;
            xform_.scale = {1.0f, 1.0f, 1.0f};
        }

        std::optional<AABB> impl_calc_bounds(const MiObjectFinder&) const override
        {
            return bounding_aabb_of(xform_.translation);
        }

        UID id_;
        std::string joint_type_name_;
        std::string user_assigned_name_;
        UID parent_;  // can be ground
        UID child_;
        Transform xform_;  // joint center
    };
}

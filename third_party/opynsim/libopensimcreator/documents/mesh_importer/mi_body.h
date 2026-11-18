#pragma once

#include <libopensimcreator/documents/mesh_importer/mi_class.h>
#include <libopensimcreator/documents/mesh_importer/mi_object_crtp.h>
#include <libopensimcreator/documents/mesh_importer/mi_object_flags.h>

#include <liboscar/maths/aabb.h>
#include <liboscar/maths/aabb_functions.h>
#include <liboscar/maths/transform.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/uid.h>

#include <iosfwd>
#include <string>
#include <string_view>

namespace osc
{
    // a body, as understood by the mesh importer
    class MiBody final : public MiObjectCRTP<MiBody> {
    public:
        MiBody(UID id, const std::string& name, const Transform& xform);

        double get_mass() const { return mass_; }

        void set_mass(double new_mass) { mass_ = new_mass; }

        Transform get_x_form() const { return xform_; }

    private:
        friend class MiObjectCRTP<MiBody>;
        static MiClass create_class();

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

        CStringView impl_get_label() const override { return name_; }

        void impl_set_label(std::string_view sv) override;

        Transform impl_get_xform(const MiObjectFinder&) const override { return get_x_form(); }

        void impl_set_xform(const MiObjectFinder&, const Transform& new_xform) override
        {
            xform_ = new_xform;
            xform_.scale = {1.0f, 1.0f, 1.0f};
        }

        std::optional<AABB> impl_calc_bounds(const MiObjectFinder&) const override
        {
            return bounding_aabb_of(xform_.translation);
        }

        UID id_;
        std::string name_;
        Transform xform_;
        double mass_ = 1.0f;  // OpenSim goes bananas if a body has a mass <= 0
    };
}

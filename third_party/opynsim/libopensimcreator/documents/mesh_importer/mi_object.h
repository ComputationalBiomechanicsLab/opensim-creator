#pragma once

#include <libopensimcreator/documents/mesh_importer/mi_crossref_descriptor.h>
#include <libopensimcreator/documents/mesh_importer/mi_crossref_direction.h>
#include <libopensimcreator/documents/mesh_importer/mi_object_flags.h>
#include <libopensimcreator/documents/mesh_importer/mi_variant_reference.h>

#include <liboscar/maths/aabb.h>
#include <liboscar/maths/euler_angles.h>
#include <liboscar/maths/quaternion.h>
#include <liboscar/maths/transform.h>
#include <liboscar/maths/vector.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/uid.h>

#include <iosfwd>
#include <memory>
#include <string_view>
#include <vector>

namespace osc
{
    class MiClass;
    class MiObjectFinder;

    // an object, as defined by the mesh importer
    class MiObject {
    protected:
        MiObject() = default;
        MiObject(const MiObject&) = default;
        MiObject(MiObject&&) noexcept = default;
        MiObject& operator=(const MiObject&) = default;
        MiObject& operator=(MiObject&&) noexcept = default;

    public:
        virtual ~MiObject() noexcept = default;

        const MiClass& get_class() const { return impl_get_class(); }

        std::unique_ptr<MiObject> clone() const { return impl_clone(); }

        MiVariantConstReference to_variant() const { return impl_to_variant(); }

        MiVariantReference to_variant() { return impl_to_variant(); }

        int get_num_cross_references() const
        {
            return static_cast<int>(impl_get_cross_references().size());
        }

        UID get_cross_reference_connectee_id(int i) const
        {
            return impl_get_cross_references().at(i).get_connectee_id();
        }

        void set_cross_reference_connectee_id(int i, UID new_id)
        {
            impl_set_cross_reference_connectee_id(i, new_id);
        }

        CStringView get_cross_reference_label(int i) const
        {
            return impl_get_cross_references().at(i).get_label();
        }

        MiCrossrefDirection get_cross_reference_direction(int i) const
        {
            return impl_get_cross_references().at(i).get_direction();
        }

        UID get_id() const { return impl_get_id(); }

        std::ostream& operator<<(std::ostream& o) const { return impl_write_to_stream(o); }

        CStringView get_label() const { return impl_get_label(); }

        void set_label(std::string_view new_label) { impl_set_label(new_label); }

        Transform get_x_form(const MiObjectFinder& lookup) const { return impl_get_xform(lookup); }
        void set_xform(const MiObjectFinder& lookup, const Transform& new_transform)
        {
            impl_set_xform(lookup, new_transform);
        }

        Vector3 get_pos(const MiObjectFinder& lookup) const
        {
            return get_x_form(lookup).translation;
        }
        void set_pos(const MiObjectFinder& lookup, const Vector3& new_pos)
        {
            set_xform(lookup, get_x_form(lookup).with_translation(new_pos));
        }

        Vector3 get_scale(const MiObjectFinder& lookup) const { return get_x_form(lookup).scale; }

        void set_scale(const MiObjectFinder& lookup, const Vector3& new_scale)
        {
            set_xform(lookup, get_x_form(lookup).with_scale(new_scale));
        }

        Quaternion rotation(const MiObjectFinder& lookup) const
        {
            return get_x_form(lookup).rotation;
        }

        void set_rotation(const MiObjectFinder& lookup, const Quaternion& new_rotation)
        {
            set_xform(lookup, get_x_form(lookup).with_rotation(new_rotation));
        }

        std::optional<AABB> calc_bounds(const MiObjectFinder& lookup) const
        {
            return impl_calc_bounds(lookup);
        }

        void apply_translation(const MiObjectFinder& lookup, const Vector3& translation)
        {
            set_pos(lookup, get_pos(lookup) + translation);
        }

        void apply_rotation(
            const MiObjectFinder& lookup,
            const EulerAngles& euler_angles,
            const Vector3& rotation_center);

        void apply_scale(const MiObjectFinder& lookup, const Vector3& scale_factors)
        {
            set_scale(lookup, get_scale(lookup) * scale_factors);
        }

        bool can_change_label() const { return impl_get_flags() & MiObjectFlags::CanChangeLabel; }

        bool can_change_position() const
        {
            return impl_get_flags() & MiObjectFlags::CanChangePosition;
        }

        bool can_change_rotation() const
        {
            return impl_get_flags() & MiObjectFlags::CanChangeRotation;
        }

        bool can_change_scale() const { return impl_get_flags() & MiObjectFlags::CanChangeScale; }

        bool can_delete() const { return impl_get_flags() & MiObjectFlags::CanDelete; }

        bool can_select() const { return impl_get_flags() & MiObjectFlags::CanSelect; }

        bool has_physical_size() const { return impl_get_flags() & MiObjectFlags::HasPhysicalSize; }

        bool is_cross_referencing(
            UID id, MiCrossrefDirection direction = MiCrossrefDirection::Both) const;

    private:
        virtual const MiClass& impl_get_class() const = 0;
        virtual std::unique_ptr<MiObject> impl_clone() const = 0;
        virtual MiVariantConstReference impl_to_variant() const = 0;
        virtual MiVariantReference impl_to_variant() = 0;
        virtual MiObjectFlags impl_get_flags() const = 0;

        virtual std::vector<MiCrossrefDescriptor> impl_get_cross_references() const { return {}; }
        virtual void impl_set_cross_reference_connectee_id(int, UID) {}

        virtual std::ostream& impl_write_to_stream(std::ostream&) const = 0;

        virtual UID impl_get_id() const = 0;

        virtual CStringView impl_get_label() const = 0;
        virtual void impl_set_label(std::string_view) {}

        virtual Transform impl_get_xform(const MiObjectFinder&) const = 0;
        virtual void impl_set_xform(const MiObjectFinder&, const Transform&) {}

        virtual std::optional<AABB> impl_calc_bounds(const MiObjectFinder&) const = 0;
    };
}

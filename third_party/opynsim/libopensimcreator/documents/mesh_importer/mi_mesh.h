#pragma once

#include <libopensimcreator/documents/mesh_importer/mi_crossref_descriptor.h>
#include <libopensimcreator/documents/mesh_importer/mi_class.h>
#include <libopensimcreator/documents/mesh_importer/mi_object_crtp.h>

#include <liboscar/graphics/mesh.h>
#include <liboscar/maths/aabb.h>
#include <liboscar/maths/transform.h>
#include <liboscar/utilities/uid.h>

#include <filesystem>
#include <iosfwd>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace osc
{
    // a mesh, as defined by the mesh importer
    class MiMesh final : public MiObjectCRTP<MiMesh> {
    public:
        MiMesh(
            UID id,
            UID attachment,  // can be MiIDs::ground()
            Mesh mesh_data,
            std::filesystem::path path);

        const Mesh& get_mesh_data() const { return mesh_data_; }

        const std::filesystem::path& get_path() const { return path_; }

        UID get_parent_id() const { return attachment_; }

        void set_parent_id(UID new_parent) { attachment_ = new_parent; }

        Transform get_x_form() const { return transform_; }

        void set_xform(const Transform& t) { transform_ = t; }

        std::optional<AABB> calc_bounds() const;

        void reload_mesh_data_from_disk();

    private:
        friend class MiObjectCRTP<MiMesh>;
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
                   MiObjectFlags::CanChangeRotation |  //
                   MiObjectFlags::CanChangeScale |     //
                   MiObjectFlags::CanDelete |          //
                   MiObjectFlags::CanSelect |          //
                   MiObjectFlags::HasPhysicalSize;
        }

        UID impl_get_id() const override { return id_; }

        std::ostream& impl_write_to_stream(std::ostream&) const override;

        CStringView impl_get_label() const override { return name_; }

        void impl_set_label(std::string_view sv) override;

        Transform impl_get_xform(const MiObjectFinder&) const override { return get_x_form(); }

        void impl_set_xform(const MiObjectFinder&, const Transform& t) override { set_xform(t); }

        std::optional<AABB> impl_calc_bounds(const MiObjectFinder&) const override
        {
            return calc_bounds();
        }

        UID id_;
        UID attachment_;  // can be MiIDs::ground()
        Transform transform_;
        osc::Mesh mesh_data_;
        std::filesystem::path path_;
        std::string name_;
    };
}

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
            UID attachment,  // can be MiIDs::Ground()
            osc::Mesh meshData,
            std::filesystem::path path
        );

        const osc::Mesh& getMeshData() const
        {
            return m_MeshData;
        }

        const std::filesystem::path& getPath() const
        {
            return m_Path;
        }

        UID getParentID() const
        {
            return m_Attachment;
        }

        void setParentID(UID newParent)
        {
            m_Attachment = newParent;
        }

        Transform getXForm() const
        {
            return m_Transform;
        }

        void setXform(const Transform& t)
        {
            m_Transform = t;
        }

        std::optional<AABB> calcBounds() const;

        void reloadMeshDataFromDisk();

    private:
        friend class MiObjectCRTP<MiMesh>;
        static MiClass CreateClass();

        std::vector<MiCrossrefDescriptor> implGetCrossReferences() const final;

        void implSetCrossReferenceConnecteeID(int i, UID id) final
        {
            if (i != 0)
            {
                throw std::runtime_error{"invalid index accessed for cross reference"};
            }
            m_Attachment = id;
        }

        MiObjectFlags implGetFlags() const final
        {
            return
                MiObjectFlags::CanChangeLabel |
                MiObjectFlags::CanChangePosition |
                MiObjectFlags::CanChangeRotation |
                MiObjectFlags::CanChangeScale |
                MiObjectFlags::CanDelete |
                MiObjectFlags::CanSelect |
                MiObjectFlags::HasPhysicalSize;
        }

        UID implGetID() const final
        {
            return m_ID;
        }

        std::ostream& implWriteToStream(std::ostream&) const final;

        CStringView implGetLabel() const final
        {
            return m_Name;
        }

        void implSetLabel(std::string_view sv) final;

        Transform implGetXform(const MiObjectFinder&) const final
        {
            return getXForm();
        }

        void implSetXform(const MiObjectFinder&, const Transform& t) final
        {
            setXform(t);
        }

        std::optional<AABB> implCalcBounds(const MiObjectFinder&) const final
        {
            return calcBounds();
        }

        UID m_ID;
        UID m_Attachment;  // can be MiIDs::Ground()
        Transform m_Transform;
        osc::Mesh m_MeshData;
        std::filesystem::path m_Path;
        std::string m_Name;
    };
}

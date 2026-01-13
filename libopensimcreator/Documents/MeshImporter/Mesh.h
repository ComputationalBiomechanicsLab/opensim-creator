#pragma once

#include <libopensimcreator/Documents/MeshImporter/CrossrefDescriptor.h>
#include <libopensimcreator/Documents/MeshImporter/MIClass.h>
#include <libopensimcreator/Documents/MeshImporter/MIObjectCRTP.h>

#include <liboscar/graphics/Mesh.h>
#include <liboscar/maths/AABB.h>
#include <liboscar/maths/Transform.h>
#include <liboscar/utils/UID.h>

#include <filesystem>
#include <iosfwd>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace osc::mi
{
    // a mesh, as defined by the mesh importer
    class Mesh final : public MIObjectCRTP<Mesh> {
    public:
        Mesh(
            UID id,
            UID attachment,  // can be MIIDs::Ground()
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
        friend class MIObjectCRTP<Mesh>;
        static MIClass CreateClass();

        std::vector<CrossrefDescriptor> implGetCrossReferences() const final;

        void implSetCrossReferenceConnecteeID(int i, UID id) final
        {
            if (i != 0)
            {
                throw std::runtime_error{"invalid index accessed for cross reference"};
            }
            m_Attachment = id;
        }

        MIObjectFlags implGetFlags() const final
        {
            return
                MIObjectFlags::CanChangeLabel |
                MIObjectFlags::CanChangePosition |
                MIObjectFlags::CanChangeRotation |
                MIObjectFlags::CanChangeScale |
                MIObjectFlags::CanDelete |
                MIObjectFlags::CanSelect |
                MIObjectFlags::HasPhysicalSize;
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

        Transform implGetXform(const IObjectFinder&) const final
        {
            return getXForm();
        }

        void implSetXform(const IObjectFinder&, const Transform& t) final
        {
            setXform(t);
        }

        std::optional<AABB> implCalcBounds(const IObjectFinder&) const final
        {
            return calcBounds();
        }

        UID m_ID;
        UID m_Attachment;  // can be MIIDs::Ground()
        Transform m_Transform;
        osc::Mesh m_MeshData;
        std::filesystem::path m_Path;
        std::string m_Name;
    };
}

#pragma once

#include <OpenSimCreator/Documents/MeshImporter/CrossrefDescriptor.h>
#include <OpenSimCreator/Documents/MeshImporter/MIClass.h>
#include <OpenSimCreator/Documents/MeshImporter/MIObjectCRTP.h>

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/AABB.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Utils/UID.h>

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

        osc::Mesh const& getMeshData() const
        {
            return m_MeshData;
        }

        std::filesystem::path const& getPath() const
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

        void setXform(Transform const& t)
        {
            m_Transform = t;
        }

        AABB calcBounds() const;

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

        Transform implGetXform(IObjectFinder const&) const final
        {
            return getXForm();
        }

        void implSetXform(IObjectFinder const&, Transform const& t) final
        {
            setXform(t);
        }

        AABB implCalcBounds(IObjectFinder const&) const final
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

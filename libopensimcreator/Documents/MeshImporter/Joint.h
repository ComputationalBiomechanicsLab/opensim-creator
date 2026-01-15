#pragma once

#include <libopensimcreator/Documents/MeshImporter/CrossrefDescriptor.h>
#include <libopensimcreator/Documents/MeshImporter/MIObjectCRTP.h>
#include <libopensimcreator/Documents/MeshImporter/MIObjectFlags.h>

#include <liboscar/maths/aabb.h>
#include <liboscar/maths/aabb_functions.h>
#include <liboscar/maths/transform.h>
#include <liboscar/utils/c_string_view.h>
#include <liboscar/utils/uid.h>

#include <cstddef>
#include <iosfwd>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace osc::mi
{
    // a joint, as defined by the mesh importer
    class Joint final : public MIObjectCRTP<Joint> {
    public:
        Joint(
            UID id,
            std::string jointTypeName,
            const std::string& userAssignedName,  // can be empty
            UID parent,
            UID child,
            const Transform& xform
        );

        CStringView getSpecificTypeName() const
        {
            return m_JointTypeName;
        }

        void setSpecificTypeName(std::string_view newName)
        {
            m_JointTypeName = newName;
        }

        UID getParentID() const
        {
            return m_Parent;
        }

        UID getChildID() const
        {
            return m_Child;
        }

        CStringView getUserAssignedName() const
        {
            return m_UserAssignedName;
        }

        Transform getXForm() const
        {
            return m_Xform;
        }

    private:
        friend class MIObjectCRTP<Joint>;
        static MIClass CreateClass();

        std::vector<CrossrefDescriptor> implGetCrossReferences() const final;

        void implSetCrossReferenceConnecteeID(int i, UID id) final
        {
            switch (i) {
            case 0:
                m_Parent = id;
                break;
            case 1:
                m_Child = id;
                break;
            default:
                throw std::runtime_error{"invalid index accessed for cross reference"};
            }
        }

        MIObjectFlags implGetFlags() const final
        {
            return
                MIObjectFlags::CanChangeLabel |
                MIObjectFlags::CanChangePosition |
                MIObjectFlags::CanChangeRotation |
                MIObjectFlags::CanDelete |
                MIObjectFlags::CanSelect;
        }

        UID implGetID() const final
        {
            return m_ID;
        }

        std::ostream& implWriteToStream(std::ostream&) const final;

        CStringView implGetLabel() const final
        {
            return m_UserAssignedName.empty() ? getSpecificTypeName() : m_UserAssignedName;
        }

        void implSetLabel(std::string_view) final;

        Transform implGetXform(const IObjectFinder&) const final
        {
            return getXForm();
        }

        void implSetXform(const IObjectFinder&, const Transform& t) final
        {
            m_Xform = t;
            m_Xform.scale = {1.0f, 1.0f, 1.0f};
        }

        std::optional<AABB> implCalcBounds(const IObjectFinder&) const final
        {
            return bounding_aabb_of(m_Xform.translation);
        }

        UID m_ID;
        std::string m_JointTypeName;
        std::string m_UserAssignedName;
        UID m_Parent;  // can be ground
        UID m_Child;
        Transform m_Xform;  // joint center
    };
}

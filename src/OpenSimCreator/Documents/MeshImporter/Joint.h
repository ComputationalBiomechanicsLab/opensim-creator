#pragma once

#include <OpenSimCreator/Documents/MeshImporter/CrossrefDescriptor.h>
#include <OpenSimCreator/Documents/MeshImporter/MIObjectCRTP.h>
#include <OpenSimCreator/Documents/MeshImporter/MIObjectFlags.h>

#include <oscar/Maths/AABB.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

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
            size_t jointTypeIdx,
            std::string const& userAssignedName,  // can be empty
            UID parent,
            UID child,
            Transform const& xform
        );

        CStringView getSpecificTypeName() const;

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

        size_t getJointTypeIndex() const
        {
            return m_JointTypeIndex;
        }

        void setJointTypeIndex(size_t i)
        {
            m_JointTypeIndex = i;
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

        Transform implGetXform(IObjectFinder const&) const final
        {
            return getXForm();
        }

        void implSetXform(IObjectFinder const&, Transform const& t) final
        {
            m_Xform = t;
            m_Xform.scale = {1.0f, 1.0f, 1.0f};
        }

        AABB implCalcBounds(IObjectFinder const&) const final
        {
            return AABB::of_point(m_Xform.position);
        }

        UID m_ID;
        size_t m_JointTypeIndex;
        std::string m_UserAssignedName;
        UID m_Parent;  // can be ground
        UID m_Child;
        Transform m_Xform;  // joint center
    };
}

#pragma once

#include <OpenSimCreator/ModelGraph/CrossrefDescriptor.hpp>
#include <OpenSimCreator/ModelGraph/SceneElCRTP.hpp>
#include <OpenSimCreator/ModelGraph/SceneElFlags.hpp>
#include <OpenSimCreator/ModelGraph/ModelGraphIDs.hpp>
#include <OpenSimCreator/ModelGraph/ModelGraphStrings.hpp>

#include <IconsFontAwesome5.h>
#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>

#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace osc
{
    // a joint scene element
    //
    // see `JointAttachment` comment for an explanation of why it's designed this way.
    class JointEl final : public SceneElCRTP<JointEl> {
    public:
        JointEl(
            UID id,
            size_t jointTypeIdx,
            std::string const& userAssignedName,  // can be empty
            UID parent,
            UID child,
            Transform const& xform);

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
        friend class SceneElCRTP<JointEl>;
        static SceneElClass CreateClass()
        {
            return
            {
                ModelGraphStrings::c_JointLabel,
                ModelGraphStrings::c_JointLabelPluralized,
                ModelGraphStrings::c_JointLabelOptionallyPluralized,
                ICON_FA_LINK,
                ModelGraphStrings::c_JointDescription,
            };
        }

        std::vector<CrossrefDescriptor> implGetCrossReferences() const final
        {
            return
            {
                {m_Parent, ModelGraphStrings::c_JointParentCrossrefName, CrossrefDirection::ToParent},
                {m_Child,  ModelGraphStrings::c_JointChildCrossrefName,  CrossrefDirection::ToChild },
            };
        }

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

        SceneElFlags implGetFlags() const final
        {
            return
                SceneElFlags::CanChangeLabel |
                SceneElFlags::CanChangePosition |
                SceneElFlags::CanChangeRotation |
                SceneElFlags::CanDelete |
                SceneElFlags::CanSelect;
        }

        UID implGetID() const final
        {
            return m_ID;
        }

        std::ostream& implWriteToStream(std::ostream& o) const final
        {
            return o << "JointEl(ID = " << m_ID
                << ", JointTypeIndex = " << m_JointTypeIndex
                << ", UserAssignedName = " << m_UserAssignedName
                << ", Parent = " << m_Parent
                << ", Child = " << m_Child
                << ", m_Transform = " << m_Xform
                << ')';
        }

        CStringView implGetLabel() const final
        {
            return m_UserAssignedName.empty() ? getSpecificTypeName() : m_UserAssignedName;
        }

        void implSetLabel(std::string_view) final;

        Transform implGetXform(ISceneElLookup const&) const final
        {
            return getXForm();
        }

        void implSetXform(ISceneElLookup const&, Transform const& t) final
        {
            m_Xform = t;
            m_Xform.scale = {1.0f, 1.0f, 1.0f};
        }

        AABB implCalcBounds(ISceneElLookup const&) const final
        {
            return AABB::OfPoint(m_Xform.position);
        }

        UID m_ID;
        size_t m_JointTypeIndex;
        std::string m_UserAssignedName;
        UID m_Parent;  // can be ground
        UID m_Child;
        Transform m_Xform;  // joint center
    };
}

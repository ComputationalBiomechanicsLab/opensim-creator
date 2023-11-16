#pragma once

#include <OpenSimCreator/ModelGraph/CrossrefDescriptor.hpp>
#include <OpenSimCreator/ModelGraph/SceneElCRTP.hpp>
#include <OpenSimCreator/ModelGraph/SceneElFlags.hpp>

#include <IconsFontAwesome5.h>
#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>

#include <iostream>
#include <string_view>
#include <vector>

namespace osc
{
    class EdgeEl final : public SceneElCRTP<EdgeEl> {
    public:
        EdgeEl(
            UID id_,
            UID firstAttachmentID_,
            UID secondAttachmentID_) :

            m_ID{id_},
            m_FirstAttachmentID{firstAttachmentID_},
            m_SecondAttachmentID{secondAttachmentID_}
        {
        }

        UID getFirstAttachmentID() const
        {
            return m_FirstAttachmentID;
        }

        UID getSecondAttachmentID() const
        {
            return m_SecondAttachmentID;
        }
    private:
        friend class SceneElCRTP<EdgeEl>;
        static SceneElClass CreateClass()
        {
            return SceneElClass
            {
                "Edge",
                "Edges",
                "Edge(s)",
                ICON_FA_ARROWS_ALT,
                "An edge between the centers of two other scene elements",
            };
        }

        std::vector<CrossrefDescriptor> implGetCrossReferences() const final
        {
            return
            {
                {m_FirstAttachmentID,  "First Point",  CrossrefDirection::ToParent},
                {m_SecondAttachmentID, "Second Point", CrossrefDirection::ToParent},
            };
        }

        void implSetCrossReferenceConnecteeID(int i, UID newAttachmentID) final
        {
            switch (i) {
            case 0:
                m_FirstAttachmentID = newAttachmentID;
                return;
            case 1:
                m_SecondAttachmentID = newAttachmentID;
                return;
            default:
                throw std::runtime_error{"invalid index passed when looking up a cross refernece"};
            }
        }

        SceneElFlags implGetFlags() const final
        {
            return
                SceneElFlags::CanChangeLabel |
                SceneElFlags::CanDelete |
                SceneElFlags::CanSelect |
                SceneElFlags::HasPhysicalSize;
        }

        UID implGetID() const final
        {
            return m_ID;
        }

        std::ostream& implWriteToStream(std::ostream& out) const final
        {
            return out << "Edge(id = " << m_ID << ", lhs = " << m_FirstAttachmentID << ", rhs = " << m_SecondAttachmentID << ')';
        }

        CStringView implGetLabel() const
        {
            return m_Label;
        }

        void implSetLabel(std::string_view newLabel) final
        {
            m_Label = newLabel;
        }

        Transform implGetXform(ISceneElLookup const&) const final;

        void implSetXform(ISceneElLookup const&, Transform const&) final
        {
            // do nothing: transform is defined by both attachments
        }

        AABB implCalcBounds(ISceneElLookup const&) const final;

        UID m_ID;
        UID m_FirstAttachmentID;
        UID m_SecondAttachmentID;
        std::string m_Label;
    };
}

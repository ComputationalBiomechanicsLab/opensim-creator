#pragma once

#include <OpenSimCreator/ModelGraph/CrossrefDescriptor.hpp>
#include <OpenSimCreator/ModelGraph/CrossrefDirection.hpp>
#include <OpenSimCreator/ModelGraph/SceneElCRTP.hpp>
#include <OpenSimCreator/ModelGraph/ModelGraphIDs.hpp>
#include <OpenSimCreator/ModelGraph/ModelGraphStrings.hpp>

#include <IconsFontAwesome5.h>
#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Utils/UID.hpp>

#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace osc
{
    // a station (point of interest)
    class StationEl final : public SceneElCRTP<StationEl> {
    public:
        StationEl(
            UID id,
            UID attachment,  // can be ModelGraphIDs::Ground()
            Vec3 const& position,
            std::string const& name
        );

        StationEl(
            UID attachment,  // can be ModelGraphIDs::Ground()
            Vec3 const& position,
            std::string const& name
        );

        UID getParentID() const
        {
            return m_Attachment;
        }

        Transform getXForm() const
        {
            return Transform{.position = m_Position};
        }

    private:
        friend class SceneElCRTP<StationEl>;
        static SceneElClass CreateClass()
        {
            return
            {
                ModelGraphStrings::c_StationLabel,
                ModelGraphStrings::c_StationLabelPluralized,
                ModelGraphStrings::c_StationLabelOptionallyPluralized,
                ICON_FA_MAP_PIN,
                ModelGraphStrings::c_StationDescription,
            };
        }

        std::vector<CrossrefDescriptor> implGetCrossReferences() const final
        {
            return
            {
                {m_Attachment, ModelGraphStrings::c_StationParentCrossrefName, CrossrefDirection::ToParent},
            };
        }

        void implSetCrossReferenceConnecteeID(int i, UID id) final
        {
            if (i != 0)
            {
                throw std::runtime_error{"invalid index accessed for cross reference"};
            }
            m_Attachment = id;
        }

        SceneElFlags implGetFlags() const final
        {
            return
                SceneElFlags::CanChangeLabel |
                SceneElFlags::CanChangePosition |
                SceneElFlags::CanDelete |
                SceneElFlags::CanSelect;
        }

        UID implGetID() const final
        {
            return m_ID;
        }

        std::ostream& implWriteToStream(std::ostream& o) const final
        {
            return o << "StationEl("
                << "ID = " << m_ID
                << ", Attachment = " << m_Attachment
                << ", Position = " << m_Position
                << ", Name = " << m_Name
                << ')';
        }

        CStringView implGetLabel() const final
        {
            return m_Name;
        }

        void implSetLabel(std::string_view) final;

        Transform implGetXform(ISceneElLookup const&) const final
        {
            return getXForm();
        }

        void implSetXform(ISceneElLookup const&, Transform const& t) final
        {
            m_Position = t.position;
        }

        AABB implCalcBounds(ISceneElLookup const&) const final
        {
            return AABB::OfPoint(m_Position);
        }

        UID m_ID;
        UID m_Attachment;  // can be ModelGraphIDs::Ground()
        Vec3 m_Position{};
        std::string m_Name;
    };
}

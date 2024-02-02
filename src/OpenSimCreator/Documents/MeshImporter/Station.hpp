#pragma once

#include <OpenSimCreator/Documents/MeshImporter/CrossrefDescriptor.hpp>
#include <OpenSimCreator/Documents/MeshImporter/MIObjectCRTP.hpp>

#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Utils/UID.hpp>

#include <iosfwd>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace osc::mi
{
    // a station (point of interest)
    class StationEl final : public MIObjectCRTP<StationEl> {
    public:
        StationEl(
            UID id,
            UID attachment,  // can be MIIDs::Ground()
            Vec3 const& position,
            std::string const& name
        );

        StationEl(
            UID attachment,  // can be MIIDs::Ground()
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
        friend class MIObjectCRTP<StationEl>;
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
            return m_Name;
        }

        void implSetLabel(std::string_view) final;

        Transform implGetXform(IObjectFinder const&) const final
        {
            return getXForm();
        }

        void implSetXform(IObjectFinder const&, Transform const& t) final
        {
            m_Position = t.position;
        }

        AABB implCalcBounds(IObjectFinder const&) const final
        {
            return AABB::OfPoint(m_Position);
        }

        UID m_ID;
        UID m_Attachment;  // can be MIIDs::Ground()
        Vec3 m_Position{};
        std::string m_Name;
    };
}

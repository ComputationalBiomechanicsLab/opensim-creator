#pragma once

#include <libopensimcreator/documents/mesh_importer/mi_crossref_descriptor.h>
#include <libopensimcreator/documents/mesh_importer/mi_object_crtp.h>

#include <liboscar/maths/aabb.h>
#include <liboscar/maths/aabb_functions.h>
#include <liboscar/maths/transform.h>
#include <liboscar/maths/vector3.h>
#include <liboscar/utilities/uid.h>

#include <iosfwd>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace osc
{
    // a station (point of interest)
    class MiStation final : public MiObjectCRTP<MiStation> {
    public:
        MiStation(
            UID id,
            UID attachment,  // can be MiIDs::Ground()
            const Vector3& position,
            const std::string& name
        );

        MiStation(
            UID attachment,  // can be MiIDs::Ground()
            const Vector3& position,
            const std::string& name
        );

        UID getParentID() const
        {
            return m_Attachment;
        }

        Transform getXForm() const
        {
            return Transform{.translation = m_Position};
        }

    private:
        friend class MiObjectCRTP<MiStation>;
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
                MiObjectFlags::CanDelete |
                MiObjectFlags::CanSelect;
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

        Transform implGetXform(const MiObjectFinder&) const final
        {
            return getXForm();
        }

        void implSetXform(const MiObjectFinder&, const Transform& t) final
        {
            m_Position = t.translation;
        }

        std::optional<AABB> implCalcBounds(const MiObjectFinder&) const final
        {
            return bounding_aabb_of(m_Position);
        }

        UID m_ID;
        UID m_Attachment;  // can be MiIDs::Ground()
        Vector3 m_Position{};
        std::string m_Name;
    };
}

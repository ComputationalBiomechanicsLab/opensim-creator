#pragma once

#include <OpenSimCreator/Documents/MeshImporter/MIClass.h>
#include <OpenSimCreator/Documents/MeshImporter/MIObjectCRTP.h>
#include <OpenSimCreator/Documents/MeshImporter/MIObjectFlags.h>

#include <oscar/Maths/AABB.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <iosfwd>
#include <string>
#include <string_view>

namespace osc::mi
{
    // a body, as understood by the mesh importer
    class Body final : public MIObjectCRTP<Body> {
    public:
        Body(
            UID id,
            std::string const& name,
            Transform const& xform
        );

        double getMass() const
        {
            return Mass;
        }

        void setMass(double newMass)
        {
            Mass = newMass;
        }

        Transform getXForm() const
        {
            return m_Xform;
        }
    private:
        friend class MIObjectCRTP<Body>;
        static MIClass CreateClass();

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
            return m_Name;
        }

        void implSetLabel(std::string_view sv) final;

        Transform implGetXform(IObjectFinder const&) const final
        {
            return getXForm();
        }

        void implSetXform(IObjectFinder const&, Transform const& newXform) final
        {
            m_Xform = newXform;
            m_Xform.scale = {1.0f, 1.0f, 1.0f};
        }

        AABB implCalcBounds(IObjectFinder const&) const final
        {
            return AABB::of_point(m_Xform.position);
        }

        UID m_ID;
        std::string m_Name;
        Transform m_Xform;
        double Mass = 1.0f;  // OpenSim goes bananas if a body has a mass <= 0
    };
}

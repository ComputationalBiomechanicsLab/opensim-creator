#pragma once

#include <libopensimcreator/Documents/MeshImporter/MIClass.h>
#include <libopensimcreator/Documents/MeshImporter/MIObjectCRTP.h>
#include <libopensimcreator/Documents/MeshImporter/MIObjectFlags.h>

#include <liboscar/Maths/AABB.h>
#include <liboscar/Maths/AABBFunctions.h>
#include <liboscar/Maths/Transform.h>
#include <liboscar/Utils/CStringView.h>
#include <liboscar/Utils/UID.h>

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
            const std::string& name,
            const Transform& xform
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

        Transform implGetXform(const IObjectFinder&) const final
        {
            return getXForm();
        }

        void implSetXform(const IObjectFinder&, const Transform& newXform) final
        {
            m_Xform = newXform;
            m_Xform.scale = {1.0f, 1.0f, 1.0f};
        }

        AABB implCalcBounds(const IObjectFinder&) const final
        {
            return bounding_aabb_of(m_Xform.translation);
        }

        UID m_ID;
        std::string m_Name;
        Transform m_Xform;
        double Mass = 1.0f;  // OpenSim goes bananas if a body has a mass <= 0
    };
}

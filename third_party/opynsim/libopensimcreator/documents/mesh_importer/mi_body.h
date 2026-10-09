#pragma once

#include <libopensimcreator/documents/mesh_importer/mi_class.h>
#include <libopensimcreator/documents/mesh_importer/mi_object_crtp.h>
#include <libopensimcreator/documents/mesh_importer/mi_object_flags.h>

#include <liboscar/maths/aabb.h>
#include <liboscar/maths/aabb_functions.h>
#include <liboscar/maths/transform.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/uid.h>

#include <iosfwd>
#include <string>
#include <string_view>

namespace osc
{
    // a body, as understood by the mesh importer
    class MiBody final : public MiObjectCRTP<MiBody> {
    public:
        MiBody(
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
        friend class MiObjectCRTP<MiBody>;
        static MiClass CreateClass();

        MiObjectFlags implGetFlags() const final
        {
            return
                MiObjectFlags::CanChangeLabel |
                MiObjectFlags::CanChangePosition |
                MiObjectFlags::CanChangeRotation |
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

        void implSetLabel(std::string_view sv) final;

        Transform implGetXform(const MiObjectFinder&) const final
        {
            return getXForm();
        }

        void implSetXform(const MiObjectFinder&, const Transform& newXform) final
        {
            m_Xform = newXform;
            m_Xform.scale = {1.0f, 1.0f, 1.0f};
        }

        std::optional<AABB> implCalcBounds(const MiObjectFinder&) const final
        {
            return bounding_aabb_of(m_Xform.translation);
        }

        UID m_ID;
        std::string m_Name;
        Transform m_Xform;
        double Mass = 1.0f;  // OpenSim goes bananas if a body has a mass <= 0
    };
}

#pragma once

#include <OpenSimCreator/Documents/MeshImporter/SceneElClass.hpp>
#include <OpenSimCreator/Documents/MeshImporter/SceneElCRTP.hpp>
#include <OpenSimCreator/Documents/MeshImporter/SceneElFlags.hpp>

#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>

#include <iosfwd>
#include <string>
#include <string_view>

namespace osc
{
    // a body scene element
    //
    // In this mesh importer, bodies are positioned + oriented in ground (see MeshEl for explanation of why).
    class BodyEl final : public SceneElCRTP<BodyEl> {
    public:
        BodyEl(
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
        friend class SceneElCRTP<BodyEl>;
        static SceneElClass CreateClass();

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

        std::ostream& implWriteToStream(std::ostream&) const final;

        CStringView implGetLabel() const final
        {
            return m_Name;
        }

        void implSetLabel(std::string_view sv) final;

        Transform implGetXform(ISceneElLookup const&) const final
        {
            return getXForm();
        }

        void implSetXform(ISceneElLookup const&, Transform const& newXform) final
        {
            m_Xform = newXform;
            m_Xform.scale = {1.0f, 1.0f, 1.0f};
        }

        AABB implCalcBounds(ISceneElLookup const&) const final
        {
            return AABB::OfPoint(m_Xform.position);
        }

        UID m_ID;
        std::string m_Name;
        Transform m_Xform;
        double Mass = 1.0f;  // OpenSim goes bananas if a body has a mass <= 0
    };
}

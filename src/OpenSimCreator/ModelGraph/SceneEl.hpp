#pragma once

#include <OpenSimCreator/ModelGraph/CrossrefDescriptor.hpp>
#include <OpenSimCreator/ModelGraph/CrossrefDirection.hpp>
#include <OpenSimCreator/ModelGraph/SceneElFlags.hpp>
#include <OpenSimCreator/ModelGraph/SceneElVariant.hpp>

#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Quat.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>

#include <iosfwd>
#include <memory>
#include <string_view>
#include <vector>

namespace osc { class ISceneElLookup; }
namespace osc { class SceneElClass; }

namespace osc
{
    // virtual scene element support
    //
    // the editor UI uses custom scene elements, rather than OpenSim types, because they have to
    // support:
    //
    // - visitor patterns (custom UI elements tailored to each known type)
    // - value semantics (undo/redo, rollbacks, etc.)
    // - groundspace manipulation (3D gizmos, drag and drop)
    // - easy UI integration (GLM datatypes, designed to be easy to dump into OpenGL, etc.)
    class SceneEl {
    protected:
        SceneEl() = default;
        SceneEl(SceneEl const&) = default;
        SceneEl(SceneEl&&) noexcept = default;
        SceneEl& operator=(SceneEl const&) = default;
        SceneEl& operator=(SceneEl&&) noexcept = default;
    public:
        virtual ~SceneEl() noexcept = default;

        SceneElClass const& getClass() const
        {
            return implGetClass();
        }

        std::unique_ptr<SceneEl> clone() const
        {
            return implClone();
        }

        ConstSceneElVariant toVariant() const
        {
            return implToVariant();
        }

        SceneElVariant toVariant()
        {
            return implToVariant();
        }

        int getNumCrossReferences() const
        {
            return static_cast<int>(implGetCrossReferences().size());
        }

        UID getCrossReferenceConnecteeID(int i) const
        {
            return implGetCrossReferences().at(i).getConnecteeID();
        }

        void setCrossReferenceConnecteeID(int i, UID newID)
        {
            implSetCrossReferenceConnecteeID(i, newID);
        }

        CStringView getCrossReferenceLabel(int i) const
        {
            return implGetCrossReferences().at(i).getLabel();
        }

        CrossrefDirection getCrossReferenceDirection(int i) const
        {
            return implGetCrossReferences().at(i).getDirection();
        }

        UID getID() const
        {
            return implGetID();
        }

        std::ostream& operator<<(std::ostream& o) const
        {
            return implWriteToStream(o);
        }

        CStringView getLabel() const
        {
            return implGetLabel();
        }

        void setLabel(std::string_view newLabel)
        {
            implSetLabel(newLabel);
        }

        Transform getXForm(ISceneElLookup const& lookup) const
        {
            return implGetXform(lookup);
        }
        void setXform(ISceneElLookup const& lookup, Transform const& newTransform)
        {
            implSetXform(lookup, newTransform);
        }

        Vec3 getPos(ISceneElLookup const& lookup) const
        {
            return getXForm(lookup).position;
        }
        void setPos(ISceneElLookup const& lookup, Vec3 const& newPos)
        {
            setXform(lookup, getXForm(lookup).withPosition(newPos));
        }

        Vec3 getScale(ISceneElLookup const& lookup) const
        {
            return getXForm(lookup).scale;
        }

        void setScale(ISceneElLookup const& lookup, Vec3 const& newScale)
        {
            setXform(lookup, getXForm(lookup).withScale(newScale));
        }

        Quat getRotation(ISceneElLookup const& lookup) const
        {
            return getXForm(lookup).rotation;
        }

        void setRotation(ISceneElLookup const& lookup, Quat const& newRotation)
        {
            setXform(lookup, getXForm(lookup).withRotation(newRotation));
        }

        AABB calcBounds(ISceneElLookup const& lookup) const
        {
            return implCalcBounds(lookup);
        }

        void applyTranslation(ISceneElLookup const& lookup, Vec3 const& translation)
        {
            setPos(lookup, getPos(lookup) + translation);
        }

        void applyRotation(
            ISceneElLookup const& lookup,
            Vec3 const& eulerAngles,
            Vec3 const& rotationCenter
        );

        void applyScale(ISceneElLookup const& lookup, Vec3 const& scaleFactors)
        {
            setScale(lookup, getScale(lookup) * scaleFactors);
        }

        bool canChangeLabel() const
        {
            return implGetFlags() & SceneElFlags::CanChangeLabel;
        }

        bool canChangePosition() const
        {
            return implGetFlags() & SceneElFlags::CanChangePosition;
        }

        bool canChangeRotation() const
        {
            return implGetFlags() & SceneElFlags::CanChangeRotation;
        }

        bool canChangeScale() const
        {
            return implGetFlags() & SceneElFlags::CanChangeScale;
        }

        bool canDelete() const
        {
            return implGetFlags() & SceneElFlags::CanDelete;
        }

        bool canSelect() const
        {
            return implGetFlags() & SceneElFlags::CanSelect;
        }

        bool hasPhysicalSize() const
        {
            return implGetFlags() & SceneElFlags::HasPhysicalSize;
        }

        bool isCrossReferencing(
            UID id,
            CrossrefDirection direction = CrossrefDirection::Both
        ) const;

    private:
        virtual SceneElClass const& implGetClass() const = 0;
        virtual std::unique_ptr<SceneEl> implClone() const = 0;
        virtual ConstSceneElVariant implToVariant() const = 0;
        virtual SceneElVariant implToVariant() = 0;
        virtual SceneElFlags implGetFlags() const = 0;

        virtual std::vector<CrossrefDescriptor> implGetCrossReferences() const { return {}; }
        virtual void implSetCrossReferenceConnecteeID(int, UID) {}

        virtual std::ostream& implWriteToStream(std::ostream&) const = 0;

        virtual UID implGetID() const = 0;

        virtual CStringView implGetLabel() const = 0;
        virtual void implSetLabel(std::string_view) {}

        virtual Transform implGetXform(ISceneElLookup const&) const = 0;
        virtual void implSetXform(ISceneElLookup const&, Transform const&) {}

        virtual AABB implCalcBounds(ISceneElLookup const&) const = 0;
    };
}

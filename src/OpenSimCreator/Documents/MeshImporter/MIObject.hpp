#pragma once

#include <OpenSimCreator/Documents/MeshImporter/CrossrefDescriptor.hpp>
#include <OpenSimCreator/Documents/MeshImporter/CrossrefDirection.hpp>
#include <OpenSimCreator/Documents/MeshImporter/MIObjectFlags.hpp>
#include <OpenSimCreator/Documents/MeshImporter/MIVariant.hpp>

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

namespace osc::mi { class IObjectFinder; }
namespace osc::mi { class MIClass; }

namespace osc::mi
{
    // an object, as defined by the mesh importer
    class MIObject {
    protected:
        MIObject() = default;
        MIObject(MIObject const&) = default;
        MIObject(MIObject&&) noexcept = default;
        MIObject& operator=(MIObject const&) = default;
        MIObject& operator=(MIObject&&) noexcept = default;
    public:
        virtual ~MIObject() noexcept = default;

        MIClass const& getClass() const
        {
            return implGetClass();
        }

        std::unique_ptr<MIObject> clone() const
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

        Transform getXForm(IObjectFinder const& lookup) const
        {
            return implGetXform(lookup);
        }
        void setXform(IObjectFinder const& lookup, Transform const& newTransform)
        {
            implSetXform(lookup, newTransform);
        }

        Vec3 getPos(IObjectFinder const& lookup) const
        {
            return getXForm(lookup).position;
        }
        void setPos(IObjectFinder const& lookup, Vec3 const& newPos)
        {
            setXform(lookup, getXForm(lookup).withPosition(newPos));
        }

        Vec3 getScale(IObjectFinder const& lookup) const
        {
            return getXForm(lookup).scale;
        }

        void setScale(IObjectFinder const& lookup, Vec3 const& newScale)
        {
            setXform(lookup, getXForm(lookup).withScale(newScale));
        }

        Quat getRotation(IObjectFinder const& lookup) const
        {
            return getXForm(lookup).rotation;
        }

        void setRotation(IObjectFinder const& lookup, Quat const& newRotation)
        {
            setXform(lookup, getXForm(lookup).withRotation(newRotation));
        }

        AABB calcBounds(IObjectFinder const& lookup) const
        {
            return implCalcBounds(lookup);
        }

        void applyTranslation(IObjectFinder const& lookup, Vec3 const& translation)
        {
            setPos(lookup, getPos(lookup) + translation);
        }

        void applyRotation(
            IObjectFinder const& lookup,
            Vec3 const& eulerAngles,
            Vec3 const& rotationCenter
        );

        void applyScale(IObjectFinder const& lookup, Vec3 const& scaleFactors)
        {
            setScale(lookup, getScale(lookup) * scaleFactors);
        }

        bool canChangeLabel() const
        {
            return implGetFlags() & MIObjectFlags::CanChangeLabel;
        }

        bool canChangePosition() const
        {
            return implGetFlags() & MIObjectFlags::CanChangePosition;
        }

        bool canChangeRotation() const
        {
            return implGetFlags() & MIObjectFlags::CanChangeRotation;
        }

        bool canChangeScale() const
        {
            return implGetFlags() & MIObjectFlags::CanChangeScale;
        }

        bool canDelete() const
        {
            return implGetFlags() & MIObjectFlags::CanDelete;
        }

        bool canSelect() const
        {
            return implGetFlags() & MIObjectFlags::CanSelect;
        }

        bool hasPhysicalSize() const
        {
            return implGetFlags() & MIObjectFlags::HasPhysicalSize;
        }

        bool isCrossReferencing(
            UID id,
            CrossrefDirection direction = CrossrefDirection::Both
        ) const;

    private:
        virtual MIClass const& implGetClass() const = 0;
        virtual std::unique_ptr<MIObject> implClone() const = 0;
        virtual ConstSceneElVariant implToVariant() const = 0;
        virtual SceneElVariant implToVariant() = 0;
        virtual MIObjectFlags implGetFlags() const = 0;

        virtual std::vector<CrossrefDescriptor> implGetCrossReferences() const { return {}; }
        virtual void implSetCrossReferenceConnecteeID(int, UID) {}

        virtual std::ostream& implWriteToStream(std::ostream&) const = 0;

        virtual UID implGetID() const = 0;

        virtual CStringView implGetLabel() const = 0;
        virtual void implSetLabel(std::string_view) {}

        virtual Transform implGetXform(IObjectFinder const&) const = 0;
        virtual void implSetXform(IObjectFinder const&, Transform const&) {}

        virtual AABB implCalcBounds(IObjectFinder const&) const = 0;
    };
}

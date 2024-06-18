#pragma once

#include <OpenSimCreator/Documents/MeshImporter/CrossrefDescriptor.h>
#include <OpenSimCreator/Documents/MeshImporter/CrossrefDirection.h>
#include <OpenSimCreator/Documents/MeshImporter/MIObjectFlags.h>
#include <OpenSimCreator/Documents/MeshImporter/MIVariant.h>

#include <oscar/Maths/AABB.h>
#include <oscar/Maths/Eulers.h>
#include <oscar/Maths/Quat.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

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
        MIObject(const MIObject&) = default;
        MIObject(MIObject&&) noexcept = default;
        MIObject& operator=(const MIObject&) = default;
        MIObject& operator=(MIObject&&) noexcept = default;
    public:
        virtual ~MIObject() noexcept = default;

        const MIClass& getClass() const
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

        Transform getXForm(const IObjectFinder& lookup) const
        {
            return implGetXform(lookup);
        }
        void setXform(const IObjectFinder& lookup, const Transform& newTransform)
        {
            implSetXform(lookup, newTransform);
        }

        Vec3 getPos(const IObjectFinder& lookup) const
        {
            return getXForm(lookup).position;
        }
        void setPos(const IObjectFinder& lookup, const Vec3& newPos)
        {
            setXform(lookup, getXForm(lookup).with_position(newPos));
        }

        Vec3 getScale(const IObjectFinder& lookup) const
        {
            return getXForm(lookup).scale;
        }

        void setScale(const IObjectFinder& lookup, const Vec3& newScale)
        {
            setXform(lookup, getXForm(lookup).with_scale(newScale));
        }

        Quat rotation(const IObjectFinder& lookup) const
        {
            return getXForm(lookup).rotation;
        }

        void set_rotation(const IObjectFinder& lookup, const Quat& newRotation)
        {
            setXform(lookup, getXForm(lookup).with_rotation(newRotation));
        }

        AABB calcBounds(const IObjectFinder& lookup) const
        {
            return implCalcBounds(lookup);
        }

        void applyTranslation(const IObjectFinder& lookup, const Vec3& translation)
        {
            setPos(lookup, getPos(lookup) + translation);
        }

        void applyRotation(
            const IObjectFinder& lookup,
            const Eulers& eulerAngles,
            const Vec3& rotationCenter
        );

        void applyScale(const IObjectFinder& lookup, const Vec3& scaleFactors)
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
        virtual const MIClass& implGetClass() const = 0;
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

        virtual Transform implGetXform(const IObjectFinder&) const = 0;
        virtual void implSetXform(const IObjectFinder&, const Transform&) {}

        virtual AABB implCalcBounds(const IObjectFinder&) const = 0;
    };
}

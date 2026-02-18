#pragma once

#include <libopensimcreator/documents/mesh_importer/mi_crossref_descriptor.h>
#include <libopensimcreator/documents/mesh_importer/mi_crossref_direction.h>
#include <libopensimcreator/documents/mesh_importer/mi_object_flags.h>
#include <libopensimcreator/documents/mesh_importer/mi_variant_reference.h>

#include <liboscar/maths/aabb.h>
#include <liboscar/maths/euler_angles.h>
#include <liboscar/maths/quaternion.h>
#include <liboscar/maths/transform.h>
#include <liboscar/maths/vector3.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/uid.h>

#include <iosfwd>
#include <memory>
#include <string_view>
#include <vector>

namespace osc { class MiObjectFinder; }
namespace osc { class MiClass; }

namespace osc
{
    // an object, as defined by the mesh importer
    class MiObject {
    protected:
        MiObject() = default;
        MiObject(const MiObject&) = default;
        MiObject(MiObject&&) noexcept = default;
        MiObject& operator=(const MiObject&) = default;
        MiObject& operator=(MiObject&&) noexcept = default;
    public:
        virtual ~MiObject() noexcept = default;

        const MiClass& getClass() const
        {
            return implGetClass();
        }

        std::unique_ptr<MiObject> clone() const
        {
            return implClone();
        }

        MiVariantConstReference toVariant() const
        {
            return implToVariant();
        }

        MiVariantReference toVariant()
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

        MiCrossrefDirection getCrossReferenceDirection(int i) const
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

        Transform getXForm(const MiObjectFinder& lookup) const
        {
            return implGetXform(lookup);
        }
        void setXform(const MiObjectFinder& lookup, const Transform& newTransform)
        {
            implSetXform(lookup, newTransform);
        }

        Vector3 getPos(const MiObjectFinder& lookup) const
        {
            return getXForm(lookup).translation;
        }
        void setPos(const MiObjectFinder& lookup, const Vector3& newPos)
        {
            setXform(lookup, getXForm(lookup).with_translation(newPos));
        }

        Vector3 getScale(const MiObjectFinder& lookup) const
        {
            return getXForm(lookup).scale;
        }

        void setScale(const MiObjectFinder& lookup, const Vector3& newScale)
        {
            setXform(lookup, getXForm(lookup).with_scale(newScale));
        }

        Quaternion rotation(const MiObjectFinder& lookup) const
        {
            return getXForm(lookup).rotation;
        }

        void set_rotation(const MiObjectFinder& lookup, const Quaternion& newRotation)
        {
            setXform(lookup, getXForm(lookup).with_rotation(newRotation));
        }

        std::optional<AABB> calcBounds(const MiObjectFinder& lookup) const
        {
            return implCalcBounds(lookup);
        }

        void applyTranslation(const MiObjectFinder& lookup, const Vector3& translation)
        {
            setPos(lookup, getPos(lookup) + translation);
        }

        void applyRotation(
            const MiObjectFinder& lookup,
            const EulerAngles& eulerAngles,
            const Vector3& rotationCenter
        );

        void applyScale(const MiObjectFinder& lookup, const Vector3& scaleFactors)
        {
            setScale(lookup, getScale(lookup) * scaleFactors);
        }

        bool canChangeLabel() const
        {
            return implGetFlags() & MiObjectFlags::CanChangeLabel;
        }

        bool canChangePosition() const
        {
            return implGetFlags() & MiObjectFlags::CanChangePosition;
        }

        bool canChangeRotation() const
        {
            return implGetFlags() & MiObjectFlags::CanChangeRotation;
        }

        bool canChangeScale() const
        {
            return implGetFlags() & MiObjectFlags::CanChangeScale;
        }

        bool canDelete() const
        {
            return implGetFlags() & MiObjectFlags::CanDelete;
        }

        bool canSelect() const
        {
            return implGetFlags() & MiObjectFlags::CanSelect;
        }

        bool hasPhysicalSize() const
        {
            return implGetFlags() & MiObjectFlags::HasPhysicalSize;
        }

        bool isCrossReferencing(
            UID id,
            MiCrossrefDirection direction = MiCrossrefDirection::Both
        ) const;

    private:
        virtual const MiClass& implGetClass() const = 0;
        virtual std::unique_ptr<MiObject> implClone() const = 0;
        virtual MiVariantConstReference implToVariant() const = 0;
        virtual MiVariantReference implToVariant() = 0;
        virtual MiObjectFlags implGetFlags() const = 0;

        virtual std::vector<MiCrossrefDescriptor> implGetCrossReferences() const { return {}; }
        virtual void implSetCrossReferenceConnecteeID(int, UID) {}

        virtual std::ostream& implWriteToStream(std::ostream&) const = 0;

        virtual UID implGetID() const = 0;

        virtual CStringView implGetLabel() const = 0;
        virtual void implSetLabel(std::string_view) {}

        virtual Transform implGetXform(const MiObjectFinder&) const = 0;
        virtual void implSetXform(const MiObjectFinder&, const Transform&) {}

        virtual std::optional<AABB> implCalcBounds(const MiObjectFinder&) const = 0;
    };
}

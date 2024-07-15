#include "ModelSelectionGizmo.h"

#include <OpenSimCreator/Documents/Model/UndoableModelActions.h>
#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>
#include <OpenSimCreator/Utils/SimTKHelpers.h>

#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/ContactGeometry.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PathPoint.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>
#include <OpenSim/Simulation/Wrap/WrapObject.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/Eulers.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/MatFunctions.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/PolarPerspectiveCamera.h>
#include <oscar/Maths/Quat.h>
#include <oscar/Maths/QuaternionFunctions.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Vec.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>
#include <oscar/Maths/VecFunctions.h>
#include <oscar/Platform/Log.h>
#include <oscar/Shims/Cpp23/utility.h>
#include <oscar/UI/ImGuizmoHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/ScopeGuard.h>
#include <oscar/Utils/Typelist.h>

#include <concepts>
#include <cstddef>
#include <memory>
#include <sstream>
#include <type_traits>
#include <utility>

namespace cpp23 = osc::cpp23;
using namespace osc;

// common/virtual manipulator data/APIs
namespace
{
    // operations that are supported by a manipulator
    enum class SupportedManipulationOpFlags : uint32_t {
        None        = 0,
        Translation = 1<<0,
        Rotation    = 1<<1,
    };

    constexpr SupportedManipulationOpFlags operator|(SupportedManipulationOpFlags lhs, SupportedManipulationOpFlags rhs)
    {
        return static_cast<SupportedManipulationOpFlags>(cpp23::to_underlying(lhs) | cpp23::to_underlying(rhs));
    }

    constexpr bool operator&(SupportedManipulationOpFlags lhs, SupportedManipulationOpFlags rhs)
    {
        return (cpp23::to_underlying(lhs) & cpp23::to_underlying(rhs)) != 0u;
    }

    // type-erased interface to an object that manipulates something in a model
    class ISelectionManipulator {
    protected:
        ISelectionManipulator() = default;
        ISelectionManipulator(const ISelectionManipulator&) = default;
        ISelectionManipulator(ISelectionManipulator&&) noexcept = default;
        ISelectionManipulator& operator=(const ISelectionManipulator&) = default;
        ISelectionManipulator& operator=(ISelectionManipulator&&) noexcept = default;
    public:
        virtual ~ISelectionManipulator() noexcept = default;

        SupportedManipulationOpFlags getSupportedManipulationOps() const
        {
            return implGetSupportedManipulationOps();
        }

        Mat4 getCurrentTransformInGround() const
        {
            return implGetCurrentTransformInGround();
        }

        void onApplyTranslation(const Vec3& translationInGround)
        {
            implOnApplyTranslation(translationInGround);
        }

        void onApplyRotation(const Eulers& eulersInLocalSpace)
        {
            implOnApplyRotation(eulersInLocalSpace);
        }

        void onSave()
        {
            implOnSave();
        }
    private:
        virtual SupportedManipulationOpFlags implGetSupportedManipulationOps() const = 0;
        virtual Mat4 implGetCurrentTransformInGround() const = 0;
        virtual void implOnApplyTranslation(const Vec3&) {}  // default to noop
        virtual void implOnApplyRotation(const Eulers&) {}  // default to noop
        virtual void implOnSave() = 0;
    };

    // abstract implementation of an `ISelectionManipulator` for the
    // given `OpenSim::Component` (`TComponent`)
    template<std::derived_from<OpenSim::Component> TComponent>
    class SelectionManipulator : public ISelectionManipulator {
    public:
        using AssociatedComponent = TComponent;

        static bool matches(const OpenSim::Component& component)
        {
            return dynamic_cast<const AssociatedComponent*>(&component);
        }
    protected:
        SelectionManipulator(
            std::shared_ptr<UndoableModelStatePair> model_,
            const AssociatedComponent& component) :

            m_Model{std::move(model_)},
            m_ComponentAbsPath{component.getAbsolutePath()}
        {
            OSC_ASSERT(m_Model != nullptr);
            OSC_ASSERT(FindComponent<AssociatedComponent>(m_Model->getModel(), m_ComponentAbsPath));
        }

        const AssociatedComponent* findSelection() const
        {
            return FindComponent<AssociatedComponent>(m_Model->getModel(), m_ComponentAbsPath);
        }

        const OpenSim::Model& getModel() const
        {
            return m_Model->getModel();
        }

        const SimTK::State& getState() const
        {
            return m_Model->getState();
        }

        UndoableModelStatePair& getUndoableModel()
        {
            return *m_Model;
        }

    private:
        // perform runtime lookup for `AssociatedComponent` and forward into concrete implementation
        Mat4 implGetCurrentTransformInGround() const final
        {
            const AssociatedComponent* maybeSelected = findSelection();
            if (not maybeSelected) {
                return identity<Mat4>();  // selection of that type does not exist in the model
            }
            return implGetCurrentTransformInGround(*maybeSelected);
        }

        // perform runtime lookup for `AssociatedComponent` and forward into concrete implementation
        void implOnApplyTranslation(const Vec3& translationInGround) final
        {
            const AssociatedComponent* maybeSelected = findSelection();
            if (not maybeSelected) {
                return;  // selection of that type does not exist in the model
            }
            implOnApplyTranslation(*maybeSelected, translationInGround);
        }

        // perform runtime lookup for `AssociatedComponent` and forward into concrete implementation
        void implOnApplyRotation(const Eulers& eulersInLocalSpace) final
        {
            const AssociatedComponent* maybeSelected = findSelection();
            if (not maybeSelected) {
                return;  // selection of that type does not exist in the model
            }
            implOnApplyRotation(*maybeSelected, eulersInLocalSpace);
        }

        void implOnSave() final
        {
            const AssociatedComponent* maybeSelected = findSelection();
            if (not maybeSelected) {
                return;  // selection of that type does not exist in the model
            }
            implOnSave(*maybeSelected);
        }

        virtual void implOnSave(const AssociatedComponent& component)
        {
            std::stringstream ss;
            ss << "transformed " << component.getName();
            getUndoableModel().commit(std::move(ss).str());
        }

        // inheritors must implement concrete manipulation methods
        virtual Mat4 implGetCurrentTransformInGround(const AssociatedComponent&) const = 0;
        virtual void implOnApplyTranslation(const AssociatedComponent&, const Vec3& translationInGround) = 0;
        virtual void implOnApplyRotation(const AssociatedComponent&, [[maybe_unused]] const Eulers& eulersInLocalSpace) {}

        std::shared_ptr<UndoableModelStatePair> m_Model;
        OpenSim::ComponentPath m_ComponentAbsPath;
    };
}

// concrete manipulator implementations
namespace
{
    // an `ISelectionManipulator` that manipulates an `OpenSim::Station`
    class StationManipulator final : public SelectionManipulator<OpenSim::Station> {
    public:
        StationManipulator(
            std::shared_ptr<UndoableModelStatePair> model_,
            const OpenSim::Station& station) :
            SelectionManipulator{std::move(model_), station}
        {}

    private:
        SupportedManipulationOpFlags implGetSupportedManipulationOps() const final
        {
            return SupportedManipulationOpFlags::Translation;
        }

        Mat4 implGetCurrentTransformInGround(
            const OpenSim::Station& station) const final
        {
            const SimTK::State& state = getState();
            Mat4 transformInGround = mat4_cast(station.getParentFrame().getRotationInGround(state));
            transformInGround[3] = Vec4{ToVec3(station.getLocationInGround(state)), 1.0f};

            return transformInGround;
        }

        void implOnApplyTranslation(
            const OpenSim::Station& station,
            const Vec3& translationInGround) final
        {
            const SimTK::Rotation parentToGroundRotation = station.getParentFrame().getRotationInGround(getState());
            const SimTK::InverseRotation& groundToParentRotation = parentToGroundRotation.invert();
            const Vec3 translationInParent = ToVec3(groundToParentRotation * ToSimTKVec3(translationInGround));

            ActionTranslateStation(getUndoableModel(), station, translationInParent);
        }

        void implOnSave(const OpenSim::Station& station) final
        {
            ActionTranslateStationAndSave(getUndoableModel(), station, {});
        }
    };

    // an `ISelectionManpulator` that manipulates an `OpenSim::PathPoint`
    class PathPointManipulator : public SelectionManipulator<OpenSim::PathPoint> {
    public:
        PathPointManipulator(
            std::shared_ptr<UndoableModelStatePair> model_,
            const OpenSim::PathPoint& pathPoint) :
            SelectionManipulator{std::move(model_), pathPoint}
        {}

    private:
        SupportedManipulationOpFlags implGetSupportedManipulationOps() const final
        {
            return SupportedManipulationOpFlags::Translation;
        }

        Mat4 implGetCurrentTransformInGround(
            const OpenSim::PathPoint& pathPoint) const final
        {
            const SimTK::State& state = getState();
            Mat4 transformInGround = mat4_cast(pathPoint.getParentFrame().getRotationInGround(state));
            transformInGround[3] = Vec4{ToVec3(pathPoint.getLocationInGround(state)), 1.0f};

            return transformInGround;
        }

        void implOnApplyTranslation(
            const OpenSim::PathPoint& pathPoint,
            const Vec3& translationInGround) final
        {
            const SimTK::Rotation parentToGroundRotation = pathPoint.getParentFrame().getRotationInGround(getState());
            const SimTK::InverseRotation& groundToParentRotation = parentToGroundRotation.invert();
            const Vec3 translationInParent = ToVec3(groundToParentRotation * ToSimTKVec3(translationInGround));

            ActionTranslatePathPoint(getUndoableModel(), pathPoint, translationInParent);
        }

        void implOnSave(const OpenSim::PathPoint& pathPoint) final
        {
            ActionTranslatePathPointAndSave(getUndoableModel(), pathPoint, {});
        }
    };

    bool IsDirectChildOfAnyJoint(const OpenSim::Model& model, const OpenSim::Frame& frame)
    {
        for (const OpenSim::Joint& joint : model.getComponentList<OpenSim::Joint>()) {
            if (&joint.getChildFrame() == &frame) {
                return true;
            }
        }
        return false;
    }

    SimTK::Rotation NegateRotation(const SimTK::Rotation& r)
    {
        return SimTK::Rotation{-SimTK::Mat33{r}, true};
    }

    // an `ISelectionManipulator` that manipulates an `OpenSim::PhysicalOffsetFrame`
    class PhysicalOffsetFrameManipulator final : public SelectionManipulator<OpenSim::PhysicalOffsetFrame> {
    public:
        PhysicalOffsetFrameManipulator(
            std::shared_ptr<UndoableModelStatePair> model_,
            const OpenSim::PhysicalOffsetFrame& pof) :
            SelectionManipulator{std::move(model_), pof},
            m_IsChildFrameOfJoint{IsDirectChildOfAnyJoint(getModel(), pof)}
        {}

    private:
        SupportedManipulationOpFlags implGetSupportedManipulationOps() const final
        {
            return SupportedManipulationOpFlags::Translation | SupportedManipulationOpFlags::Rotation;
        }

        Mat4 implGetCurrentTransformInGround(
            const OpenSim::PhysicalOffsetFrame& pof) const final
        {
            if (m_IsChildFrameOfJoint) {
                // if the POF that's being edited is the child frame of a joint then
                // its offset/orientation is constrained to be in the same location/orientation
                // as the joint's parent frame (plus coordinate transforms)
                auto t = pof.getTransformInGround(getState());
                t.updR() = NegateRotation(t.R());
                t.updP() = pof.getParentFrame().getPositionInGround(getState());
                return ToMat4x4(t);
            }
            else {
                return ToMat4x4(pof.getTransformInGround(getState()));
            }
        }

        void implOnApplyTranslation(
            const OpenSim::PhysicalOffsetFrame& pof,
            const Vec3& translationInGround) final
        {
            SimTK::Rotation parentToGroundRotation = pof.getParentFrame().getRotationInGround(getState());
            if (m_IsChildFrameOfJoint) {
                parentToGroundRotation = NegateRotation(parentToGroundRotation);
            }
            const SimTK::InverseRotation& groundToParentRotation = parentToGroundRotation.invert();
            const SimTK::Vec3 parentTranslation = groundToParentRotation * ToSimTKVec3(translationInGround);
            const SimTK::Vec3& eulersInPofFrame = pof.get_orientation();

            ActionTransformPof(
                getUndoableModel(),
                pof,
                ToVec3(parentTranslation),
                ToVec3(eulersInPofFrame)
            );
        }

        void implOnApplyRotation(
            const OpenSim::PhysicalOffsetFrame& pof,
            const Eulers& eulersInLocalSpace) final
        {
            const OpenSim::Frame& parent = pof.getParentFrame();
            const SimTK::State& state = getState();

            const Quat deltaRotationInGround = to_worldspace_rotation_quat(m_IsChildFrameOfJoint ? -eulersInLocalSpace : eulersInLocalSpace);
            const Quat oldRotationInGround = ToQuat(pof.getRotationInGround(state));
            const Quat parentRotationInGround = ToQuat(parent.getRotationInGround(state));
            const Quat newRotationInGround = normalize(deltaRotationInGround * oldRotationInGround);
            const Quat newRotationInParent = inverse(parentRotationInGround) * newRotationInGround;

            ActionTransformPof(
                getUndoableModel(),
                pof,
                Vec3{},  // no translation delta
                extract_eulers_xyz(newRotationInParent)
            );
        }

        bool m_IsChildFrameOfJoint = false;
    };

    // manipulator for `OpenSim::WrapObject`
    class WrapObjectManipulator final : public SelectionManipulator<OpenSim::WrapObject> {
    public:
        WrapObjectManipulator(
            std::shared_ptr<UndoableModelStatePair> model_,
            const OpenSim::WrapObject& wo) :
            SelectionManipulator{std::move(model_), wo}
        {}

    private:
        SupportedManipulationOpFlags implGetSupportedManipulationOps() const final
        {
            return SupportedManipulationOpFlags::Rotation | SupportedManipulationOpFlags::Translation;
        }

        Mat4 implGetCurrentTransformInGround(
            const OpenSim::WrapObject& wrapObj) const final
        {
            const SimTK::Transform& wrapToFrame = wrapObj.getTransform();
            const SimTK::Transform frameToGround = wrapObj.getFrame().getTransformInGround(getState());
            const SimTK::Transform wrapToGround = frameToGround * wrapToFrame;

            return ToMat4x4(wrapToGround);
        }

        void implOnApplyTranslation(
            const OpenSim::WrapObject& wrapObj,
            const Vec3& translationInGround) final
        {
            const SimTK::Rotation frameToGroundRotation = wrapObj.getFrame().getTransformInGround(getState()).R();
            const SimTK::InverseRotation& groundToFrameRotation = frameToGroundRotation.invert();
            const Vec3 translationInPofFrame = ToVec3(groundToFrameRotation * ToSimTKVec3(translationInGround));

            ActionTransformWrapObject(
                getUndoableModel(),
                wrapObj,
                translationInPofFrame,
                ToVec3(wrapObj.get_xyz_body_rotation())
            );
        }

        void implOnApplyRotation(
            const OpenSim::WrapObject& wrapObj,
            const Eulers& eulersInLocalSpace) final
        {
            const OpenSim::Frame& parent = wrapObj.getFrame();
            const SimTK::State& state = getState();

            const Quat deltaRotationInGround = to_worldspace_rotation_quat(eulersInLocalSpace);
            const Quat oldRotationInGround = ToQuat(parent.getTransformInGround(state).R() * wrapObj.getTransform().R());
            const Quat newRotationInGround = normalize(deltaRotationInGround * oldRotationInGround);

            const Quat parentRotationInGround = ToQuat(parent.getRotationInGround(state));
            const Quat newRotationInParent = inverse(parentRotationInGround) * newRotationInGround;

            ActionTransformWrapObject(
                getUndoableModel(),
                wrapObj,
                Vec3{},  // no translation
                extract_eulers_xyz(newRotationInParent)
            );
        }
    };

    // manipulator for `OpenSim::ContactGeometry`
    class ContactGeometryManipulator final : public SelectionManipulator<OpenSim::ContactGeometry> {
    public:
        ContactGeometryManipulator(
            std::shared_ptr<UndoableModelStatePair> model_,
            const OpenSim::ContactGeometry& contactGeom) :
            SelectionManipulator{std::move(model_), contactGeom}
        {}

    private:
        SupportedManipulationOpFlags implGetSupportedManipulationOps() const final
        {
            return SupportedManipulationOpFlags::Rotation | SupportedManipulationOpFlags::Translation;
        }

        Mat4 implGetCurrentTransformInGround(
            const OpenSim::ContactGeometry& contactGeom) const final
        {
            const SimTK::Transform wrapToFrame = contactGeom.getTransform();
            const SimTK::Transform frameToGround = contactGeom.getFrame().getTransformInGround(getState());
            const SimTK::Transform wrapToGround = frameToGround * wrapToFrame;

            return ToMat4x4(wrapToGround);
        }

        void implOnApplyTranslation(
            const OpenSim::ContactGeometry& contactGeom,
            const Vec3& deltaTranslationInGround) final
        {
            const SimTK::Rotation frameToGroundRotation = contactGeom.getFrame().getTransformInGround(getState()).R();
            const SimTK::InverseRotation& groundToFrameRotation = frameToGroundRotation.invert();
            const Vec3 translationInPofFrame = ToVec3(groundToFrameRotation * ToSimTKVec3(deltaTranslationInGround));

            ActionTransformContactGeometry(
                getUndoableModel(),
                contactGeom,
                translationInPofFrame,
                ToVec3(contactGeom.get_orientation())
            );
        }

        void implOnApplyRotation(
            const OpenSim::ContactGeometry& contactGeom,
            const Eulers& deltaEulerRadiansInGround) final
        {
            const OpenSim::Frame& parent = contactGeom.getFrame();
            const SimTK::State& state = getState();

            const Quat deltaRotationInGround = to_worldspace_rotation_quat(deltaEulerRadiansInGround);
            const Quat oldRotationInGround = ToQuat(parent.getTransformInGround(state).R() * contactGeom.getTransform().R());
            const Quat newRotationInGround = normalize(deltaRotationInGround * oldRotationInGround);

            const Quat parentRotationInGround = ToQuat(parent.getRotationInGround(state));
            const Quat newRotationInParent = inverse(parentRotationInGround) * newRotationInGround;

            ActionTransformContactGeometry(
                getUndoableModel(),
                contactGeom,
                Vec3{},  // no translation
                extract_eulers_xyz(newRotationInParent)
            );
        }
    };

    // an `ISelectionManipulator` that manipulates an `OpenSim::Joint` in the case where
    // both sides of the joint are connected to `OpenSim::PhysicalOffsetFrame`s
    class JointManipulator final : public SelectionManipulator<OpenSim::Joint> {
    public:
        static bool matches(const OpenSim::Component& component)
        {
            const auto* joint = dynamic_cast<const OpenSim::Joint*>(&component);
            if (not joint) {
                return false;  // it isn't an `OpenSim::Joint`
            }

            if (not dynamic_cast<const OpenSim::PhysicalOffsetFrame*>(&joint->getParentFrame())) {
                return false;  // the parent frame isn't an `OpenSim::PhysicalOffsetFrame`
            }

            if (not dynamic_cast<const OpenSim::PhysicalOffsetFrame*>(&joint->getChildFrame())) {
                return false;  // the parent isn't an `OpenSim::PhysicalOffsetFrame`
            }

            return true;
        }

        JointManipulator(
            std::shared_ptr<UndoableModelStatePair> model_,
            const OpenSim::Joint& joint) :
            SelectionManipulator{std::move(model_), joint}
        {}

    private:
        SupportedManipulationOpFlags implGetSupportedManipulationOps() const final
        {
            return SupportedManipulationOpFlags::Translation | SupportedManipulationOpFlags::Rotation;
        }

        Mat4 implGetCurrentTransformInGround(const OpenSim::Joint& joint) const final
        {
            // present the "joint center" as equivalent to the parent frame
            return ToMat4x4(joint.getParentFrame().getTransformInGround(getState()));
        }

        void implOnApplyTranslation(const OpenSim::Joint& joint, const Vec3& deltaTranslationInGround) final
        {
            applyTransform(joint, deltaTranslationInGround, Eulers{});
        }

        void implOnApplyRotation([[maybe_unused]] const OpenSim::Joint& joint, [[maybe_unused]] const Eulers& deltaEulersInLocalSpace) final
        {
            applyTransform(joint, Vec3{}, deltaEulersInLocalSpace);
        }

        void applyTransform(
            const OpenSim::Joint& joint,
            const Vec3& deltaTranslationInGround,
            const Eulers& deltaEulersInLocalSpace)
        {
            // in order to move a joint center without every child also moving around, we need to:
            //
            // STEP 1) move the parent offset frame (as normal)
            // STEP 2) figure out what transform the child offset frame would need to have in
            //         order for its parent (confusing, eh) to not move
            //
            // this ended up being a headfuck, but I figured out that the easiest way to tackle this
            // is to carefully track+name each frame definition and trust in god by using linear
            // algebra to figure out the rest. So, given:
            //
            // - M_cpof1                    joint child offset frame to its parent transform (1: BEFORE)
            // - M_j                        joint child-to-parent transform
            // - M_ppof1                    joint parent offset frame to its parent transform (1: BEFORE)
            // - M_ppof2                    joint parent offset frame to its parent transform (2: AFTER)
            // - M_cpof2  **WE WANT THIS**  joint child offset frame to its parent transform (2: AFTER)
            // - vcp                        an example quantity, expressed in the child's parent frame (e.g. a body)
            // - vjp                        the same example quantity, but expressed in the joint's parent frame
            //
            // computing `vjp` from `vcp` involves going up the kinematic chain:
            //
            //     vjp = M_ppof1 * M_j * M_cpof1^-1 * vcp
            //
            // now, our goal is to apply STEP 1 (M_ppof1 --> M_ppof2) and calculate a new `M_cpof2` such that
            // quantities expressed in a child body (e.g. `vcp`) do not move in the scene. I.e.:
            //
            //     vjp = M_ppof1 * M_j * M_cpof1^-1 * vcp = M_ppof2 * M_j * M_cpof2^-1 * vcp
            //
            // simplifying, and dropping the pretext of using the transforms to transform a particular point:
            //
            //     M_ppof1 * M_j * M_cpof1^-1 = M_ppof2 * M_j * M_cpof2^-1
            //     M_ppof2^-1 * M_ppof1 * M_j * M_cpof1^-1 = M_j * M_cpof2^-1
            //     M_j^-1 * M_ppof2^-1 * M_ppof1 * M_j * M_cpof1^-1 = M_cpof2^-1
            //     M_cpof2^-1 = M_j^-1 * M_ppof2^-1 * M_ppof1 * M_j * M_cpof1^-1
            //     M_cpof2 = (M_j^-1 * M_ppof2^-1 * M_ppof1 * M_j * M_cpof1^-1)^-1;
            //
            // the code below essentially collects all of this information up to figure out `M_cpof2` and stuff
            // it into the child `OpenSim::PhysicalOffsetFrame`

            const auto& parentPOF = dynamic_cast<const OpenSim::PhysicalOffsetFrame&>(joint.getParentFrame());
            const auto& childPOF = dynamic_cast<const OpenSim::PhysicalOffsetFrame&>(joint.getChildFrame());

            // get BEFORE transforms
            const auto M_j = childPOF.findTransformBetween(getState(), parentPOF);
            const auto M_ppof1 = parentPOF.findTransformBetween(getState(), parentPOF.getParentFrame());
            const auto M_cpof1 = childPOF.findTransformBetween(getState(), childPOF.getParentFrame());

            // STEP 1) move the parent offset frame (as normal)
            {
                const SimTK::Rotation R = ToSimTKRotation(deltaEulersInLocalSpace);
                const auto& M_p = M_ppof1;
                const auto M_o = parentPOF.getParentFrame().getTransformInGround(getState());
                const auto M_n = ToSimTKTransform(deltaEulersInLocalSpace, deltaTranslationInGround);

                const SimTK::Rotation X_r = M_p.R() * R;
                const SimTK::Vec3 X_p = M_p.p() + ToSimTKVec3(deltaTranslationInGround);
                const SimTK::Transform X{X_r, X_p};
                ActionTransformPof(
                    getUndoableModel(),
                    parentPOF,
                    ToVec3(X.p() - M_ppof1.p()),
                    ToVec3(X.R().convertRotationToBodyFixedXYZ())
                );
            }

            // STEP 2) figure out what transform the child offset frame would need to have in
            //         order for its parent (confusing, eh) to not move

            // get AFTER transforms
            const auto M_ppof2 = parentPOF.findTransformBetween(getState(), parentPOF.getParentFrame());

            // caclulate `M_cpof2` (i.e. the desired new child transform)
            const SimTK::Transform M_cpof2 = (M_j.invert() * M_ppof2.invert() * M_ppof1 * M_j * M_cpof1.invert()).invert();

            // decompse `M_cpof2` into the child `OpenSim::PhysicalOffsetFrame`'s properties
            ActionTransformPofV2(
                getUndoableModel(),
                childPOF,
                ToVec3(M_cpof2.p()),
                ToVec3(M_cpof2.R().convertRotationToBodyFixedXYZ())
            );
        }
    };

    // a compile-time `Typelist` containing all concrete implementations of
    // `ISelectionManipulator`
    using ManipulatorList = Typelist<
        StationManipulator,
        PathPointManipulator,
        PhysicalOffsetFrameManipulator,
        WrapObjectManipulator,
        ContactGeometryManipulator,
        JointManipulator
    >;
}

// drawing/rendering code
namespace
{
    // state associated with one drawing command
    struct GizmoUIState final {
        void* gizmoID;
        const PolarPerspectiveCamera& camera;
        const Rect& viewportRect;
        ImGuizmo::OPERATION operation;
        ImGuizmo::MODE mode;
        const std::shared_ptr<UndoableModelStatePair>& model;
        const OpenSim::Component& component;
        bool& wasUsingLastFrameStorage;
    };

    // draws the gizmo overlay using the given `ISelectionManipulator`
    void DrawGizmoOverlayInner(
        GizmoUIState& st,
        ISelectionManipulator& manipulator)
    {
        // figure out whether the gizmo should even be drawn
        {
            const SupportedManipulationOpFlags flags = manipulator.getSupportedManipulationOps();
            if (st.operation == ImGuizmo::TRANSLATE && !(flags & SupportedManipulationOpFlags::Translation)) {
                return;
            }
            if (st.operation == ImGuizmo::ROTATE && !(flags & SupportedManipulationOpFlags::Rotation)) {
                return;
            }
        }
        // else: it's a supported operation and the gizmo should be drawn

        // important: necessary for multi-viewport gizmos
        // also important: don't use ui::get_id(), because it uses an ID stack and we might want to know if "isover" etc. is true outside of a window
        ImGuizmo::SetID(static_cast<int>(std::hash<void*>{}(st.gizmoID)));
        const ScopeGuard g{[]() { ImGuizmo::SetID(-1); }};

        ImGuizmo::SetRect(
            st.viewportRect.p1.x,
            st.viewportRect.p1.y,
            dimensions_of(st.viewportRect).x,
            dimensions_of(st.viewportRect).y
        );
        ImGuizmo::SetDrawlist(ui::get_panel_draw_list());
        ImGuizmo::AllowAxisFlip(false);

        // use rotation from the parent, translation from station
        Mat4 currentXformInGround = manipulator.getCurrentTransformInGround();
        Mat4 deltaInGround;

        ui::set_gizmo_style_to_osc_standard();
        const bool gizmoWasManipulatedByUser = ImGuizmo::Manipulate(
            value_ptr(st.camera.view_matrix()),
            value_ptr(st.camera.projection_matrix(aspect_ratio_of(st.viewportRect))),
            st.operation,
            st.mode,
            value_ptr(currentXformInGround),
            value_ptr(deltaInGround),
            nullptr,
            nullptr,
            nullptr
        );

        const bool isUsingThisFrame = ImGuizmo::IsUsing();
        const bool wasUsingLastFrame = st.wasUsingLastFrameStorage;
        st.wasUsingLastFrameStorage = isUsingThisFrame;  // update cached state

        if (wasUsingLastFrame and not isUsingThisFrame) {
            // user finished interacting, save model
            manipulator.onSave();
        }

        if (not gizmoWasManipulatedByUser) {
            return;  // user is not interacting, so no changes to apply
        }
        // else: apply in-place change to model

        // decompose the overall transformation into component parts
        Vec3 translationInGround{};
        Vec3 rotationInLocalSpaceDegrees{};
        Vec3 scaleInLocalSpace{};
        ImGuizmo::DecomposeMatrixToComponents(
            value_ptr(deltaInGround),
            value_ptr(translationInGround),
            value_ptr(rotationInLocalSpaceDegrees),
            value_ptr(scaleInLocalSpace)
        );
        Eulers rotationInLocalSpace = Vec<3, Degrees>(rotationInLocalSpaceDegrees);

        if (st.operation == ImGuizmo::TRANSLATE) {
            manipulator.onApplyTranslation(translationInGround);
        }
        else if (st.operation == ImGuizmo::ROTATE) {
            manipulator.onApplyRotation(rotationInLocalSpace);
        }
    }

    template<std::derived_from<ISelectionManipulator> ConcreteManipulator>
    bool TryManipulateComponentWithManipulator(
        GizmoUIState& st)
    {
        if (not ConcreteManipulator::matches(st.component)) {
            return false;
        }

        const auto* downcasted = dynamic_cast<const ConcreteManipulator::AssociatedComponent*>(&st.component);
        if (not downcasted) {
            return false;
        }

        ConcreteManipulator manipulator{st.model, *downcasted};
        DrawGizmoOverlayInner(st, manipulator);
        return true;
    }

    template<typename... ConcreteManipulator>
    void TryManipulateComponentWithMatchingManipulator(
        GizmoUIState& st,
        Typelist<ConcreteManipulator...>)
    {
        (TryManipulateComponentWithManipulator<ConcreteManipulator>(st) || ...);
    }
}

osc::ModelSelectionGizmo::ModelSelectionGizmo(std::shared_ptr<UndoableModelStatePair> model_) :
    m_Model{std::move(model_)}
{}
osc::ModelSelectionGizmo::ModelSelectionGizmo(const ModelSelectionGizmo&) = default;
osc::ModelSelectionGizmo::ModelSelectionGizmo(ModelSelectionGizmo&&) noexcept = default;
osc::ModelSelectionGizmo& osc::ModelSelectionGizmo::operator=(const ModelSelectionGizmo&) = default;
osc::ModelSelectionGizmo& osc::ModelSelectionGizmo::operator=(ModelSelectionGizmo&&) noexcept = default;
osc::ModelSelectionGizmo::~ModelSelectionGizmo() noexcept = default;

bool osc::ModelSelectionGizmo::isUsing() const
{
    ImGuizmo::SetID(static_cast<int>(std::hash<const ModelSelectionGizmo*>{}(this)));
    const bool rv = ImGuizmo::IsUsing();
    ImGuizmo::SetID(-1);
    return rv;
}

bool osc::ModelSelectionGizmo::isOver() const
{
    ImGuizmo::SetID(static_cast<int>(std::hash<const ModelSelectionGizmo*>{}(this)));
    const bool rv = ImGuizmo::IsOver();
    ImGuizmo::SetID(-1);
    return rv;
}

bool osc::ModelSelectionGizmo::handleKeyboardInputs()
{
    return ui::update_gizmo_state_from_keyboard(m_GizmoOperation, m_GizmoMode);
}

void osc::ModelSelectionGizmo::onDraw(
    const Rect& screenRect,
    const PolarPerspectiveCamera& camera)
{
    const OpenSim::Component* selected = m_Model->getSelected();
    if (not selected) {
        return;
    }

    GizmoUIState st{
        .gizmoID = this,
        .camera = camera,
        .viewportRect = screenRect,
        .operation = m_GizmoOperation,
        .mode = m_GizmoMode,
        .model = m_Model,
        .component = *selected,
        .wasUsingLastFrameStorage = m_WasUsingGizmoLastFrame
    };
    TryManipulateComponentWithMatchingManipulator(st, ManipulatorList{});
}

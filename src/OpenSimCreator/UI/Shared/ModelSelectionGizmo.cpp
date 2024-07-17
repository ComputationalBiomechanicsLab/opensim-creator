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
#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/ScopeGuard.h>
#include <oscar/Utils/StringHelpers.h>
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

        void onApplyTransform(const ui::GizmoTransform& transformInGround)
        {
            implOnApplyTransform(transformInGround);
        }

        void onSave()
        {
            implOnSave();
        }
    private:
        virtual SupportedManipulationOpFlags implGetSupportedManipulationOps() const = 0;
        virtual Mat4 implGetCurrentTransformInGround() const = 0;
        virtual void implOnApplyTransform(const ui::GizmoTransform&) = 0;
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
        void implOnApplyTransform(const ui::GizmoTransform& transformInGround) final
        {
            const AssociatedComponent* maybeSelected = findSelection();
            if (not maybeSelected) {
                return;  // selection of that type does not exist in the model
            }
            implOnApplyTransform(*maybeSelected, transformInGround);
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
        virtual void implOnApplyTransform(const AssociatedComponent&, const ui::GizmoTransform& transformInGround) = 0;

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

        void implOnApplyTransform(
            const OpenSim::Station& station,
            const ui::GizmoTransform& transformInGround) final
        {
            // ignores `scale` and `position`

            const SimTK::Rotation parentToGroundRotation = station.getParentFrame().getRotationInGround(getState());
            const SimTK::InverseRotation& groundToParentRotation = parentToGroundRotation.invert();
            const Vec3 translationInParent = ToVec3(groundToParentRotation * ToSimTKVec3(transformInGround.position));

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

        void implOnApplyTransform(
            const OpenSim::PathPoint& pathPoint,
            const ui::GizmoTransform& transformInGround) final
        {
            const SimTK::Rotation parentToGroundRotation = pathPoint.getParentFrame().getRotationInGround(getState());
            const SimTK::InverseRotation& groundToParentRotation = parentToGroundRotation.invert();
            const Vec3 translationInParent = ToVec3(groundToParentRotation * ToSimTKVec3(transformInGround.position));

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
                return ToMat4x4(pof.getParentFrame().getTransformInGround(getState()));
            }
            else {
                return ToMat4x4(pof.getTransformInGround(getState()));
            }
        }

        void implOnApplyTransform(
            const OpenSim::PhysicalOffsetFrame& pof,
            const ui::GizmoTransform& transformInGround) final
        {
            if (m_IsChildFrameOfJoint) {
                // the difference here is that the child frame's translation/rotation in ground
                // is dictated by joints, but the user is manipulating stuff "as-if" they were
                // editing the parent frame
                //
                // M_n * M_pofg * M_p^-1 * v_parent = M_pofg * X^-1 * v_parent
                //
                // - M_n        user-enacted transformation in ground
                // - M_pofg     pof-to-ground transform
                // - M_p        pof-to-parent transform
                // - v_parent   a point, expressed in the pof's parent

                const SimTK::Transform M_n = {ToSimTKRotation(transformInGround.rotation), ToSimTKVec3(transformInGround.position)};
                const SimTK::Transform M_pofg = pof.getTransformInGround(getState());
                const SimTK::Transform M_p = pof.findTransformBetween(getState(), pof.getParentFrame());
                const SimTK::Transform X = (M_pofg.invert() * M_n * M_pofg * M_p.invert()).invert();

                ActionTransformPofV2(
                    getUndoableModel(),
                    pof,
                    ToVec3(X.p()),
                    ToVec3(X.R().convertRotationToBodyFixedXYZ())
                );
            }
            else {
                // this might disgust you to hear, but the easiest way to figure this out is by
                // getting out a pen and paper and solving the following for X:
                //
                //     M_n * M_g * M_p * v_pof = M_g * X * v_pof
                //
                // where:
                //
                // - M_n        user-enacted transformation in ground
                // - M_g        parent-to-ground transform
                // - M_p        pof-to-parent transform
                // - v_pof      a point, expressed in the pof

                const SimTK::Transform M_n = {ToSimTKRotation(transformInGround.rotation), ToSimTKVec3(transformInGround.position)};
                const SimTK::Transform M_g = pof.getParentFrame().getTransformInGround(getState());
                const SimTK::Transform M_p = pof.findTransformBetween(getState(), pof.getParentFrame());
                const SimTK::Transform X = M_g.invert() * M_n * M_g * M_p;

                ActionTransformPofV2(
                    getUndoableModel(),
                    pof,
                    ToVec3(X.p()),
                    ToVec3(X.R().convertRotationToBodyFixedXYZ())
                );
            }
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

        void implOnApplyTransform(
            const OpenSim::WrapObject& wrapObj,
            const ui::GizmoTransform& transformInGround) final
        {
            // solve for X:
            //
            //     M_n * M_g * M_w * v = M_g * X * v
            //
            // where:
            //
            // - M_n   user-enacted transform in ground
            // - M_g   parent-frame-to-ground transform
            // - M_w   wrap object local transform

            const SimTK::Transform M_n = {ToSimTKRotation(transformInGround.rotation), ToSimTKVec3(transformInGround.position)};
            const SimTK::Transform M_g = wrapObj.getFrame().getTransformInGround(getState());
            const SimTK::Transform M_w = wrapObj.getTransform();
            const SimTK::Transform X = M_g.invert() * M_n * M_g * M_w;

            ActionTransformWrapObject(
                getUndoableModel(),
                wrapObj,
                ToVec3(X.p() - M_w.p()),
                ToVec3(X.R().convertRotationToBodyFixedXYZ())
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

        void implOnApplyTransform(
            const OpenSim::ContactGeometry& contactGeom,
            const ui::GizmoTransform& transformInGround) final
        {
            // solve for X:
            //
            //     M_n * M_g * M_w * v = M_g * X * v
            //
            // where:
            //
            // - M_n   user-enacted transform in ground
            // - M_g   parent-frame-to-ground transform
            // - M_w   contact geometry local transform

            const SimTK::Transform M_n = {ToSimTKRotation(transformInGround.rotation), ToSimTKVec3(transformInGround.position)};
            const SimTK::Transform M_g = contactGeom.getFrame().getTransformInGround(getState());
            const SimTK::Transform M_w = contactGeom.getTransform();
            const SimTK::Transform X = M_g.invert() * M_n * M_g * M_w;

            ActionTransformContactGeometry(
                getUndoableModel(),
                contactGeom,
                ToVec3(X.p() - M_w.p()),
                ToVec3(X.R().convertRotationToBodyFixedXYZ())
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

        void implOnApplyTransform(
            const OpenSim::Joint& joint,
            const ui::GizmoTransform& transformInGround) final
        {
            // in order to move a joint center without every child also moving around, we need to:
            //
            // STEP 1) move the parent offset frame (as normal)
            // STEP 2) figure out what transform the child offset frame would need to have in
            //         order for its parent (confusing, eh) to not move
            //
            // the easiest way to tackle this is to carefully track+name each frame definition
            // and trust in god by using linear algebra to figure out the rest. So, given:
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
            const SimTK::Transform M_j = childPOF.findTransformBetween(getState(), parentPOF);
            const SimTK::Transform M_ppof1 = parentPOF.findTransformBetween(getState(), parentPOF.getParentFrame());
            const SimTK::Transform M_cpof1 = childPOF.findTransformBetween(getState(), childPOF.getParentFrame());

            // STEP 1) move the parent offset frame (as normal)
            {
                // M_n * M_g * M_ppof1 * v = M_g * X * v
                const SimTK::Transform M_n{ToSimTKRotation(transformInGround.rotation), ToSimTKVec3(transformInGround.position)};
                const SimTK::Transform M_g = parentPOF.getParentFrame().getTransformInGround(getState());
                const SimTK::Transform X = M_g.invert() * M_n * M_g * M_ppof1;
                ActionTransformPofV2(
                    getUndoableModel(),
                    parentPOF,
                    ToVec3(X.p()),
                    ToVec3(X.R().convertRotationToBodyFixedXYZ())
                );
            }

            // STEP 2) figure out what transform the child offset frame would need to have in
            //         order for its parent (confusing, eh) to not move

            // get AFTER transforms
            const SimTK::Transform M_ppof2 = parentPOF.findTransformBetween(getState(), parentPOF.getParentFrame());

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
    // draws the gizmo overlay using the given `ISelectionManipulator`
    void DrawGizmoOverlayInner(
        ui::Gizmo& gizmo,
        const Rect& screenRect,
        const PolarPerspectiveCamera& camera,
        ISelectionManipulator& manipulator)
    {
        // figure out whether the gizmo should even be drawn
        {
            const SupportedManipulationOpFlags flags = manipulator.getSupportedManipulationOps();
            if (gizmo.operation() == ui::GizmoOperation::Translate && !(flags & SupportedManipulationOpFlags::Translation)) {
                return;
            }
            if (gizmo.operation() == ui::GizmoOperation::Rotate && !(flags & SupportedManipulationOpFlags::Rotation)) {
                return;
            }
        }

        // draw the manipulator
        Mat4 modelMatrix = manipulator.getCurrentTransformInGround();
        const auto userEditInGround = gizmo.draw(
            modelMatrix,
            camera.view_matrix(),
            camera.projection_matrix(aspect_ratio_of(screenRect)),
            screenRect
        );

        if (userEditInGround) {
            // propagate user edit to the model via the `ISelectionManipulator` interface
            manipulator.onApplyTransform(*userEditInGround);
        }

        // once the user stops using the manipulator, save the changes
        if (gizmo.was_using() and not gizmo.is_using()) {
            manipulator.onSave();
        }
    }

    template<std::derived_from<ISelectionManipulator> ConcreteManipulator>
    bool TryManipulateComponentWithManipulator(
        ui::Gizmo& gizmo,
        const Rect& screenRect,
        const PolarPerspectiveCamera& camera,
        const std::shared_ptr<UndoableModelStatePair>& model,
        const OpenSim::Component& selected)
    {
        if (not ConcreteManipulator::matches(selected)) {
            return false;
        }

        const auto* downcasted = dynamic_cast<const typename ConcreteManipulator::AssociatedComponent*>(&selected);
        if (not downcasted) {
            return false;
        }

        ConcreteManipulator manipulator{model, *downcasted};
        DrawGizmoOverlayInner(gizmo, screenRect, camera, manipulator);
        return true;
    }

    template<typename... ConcreteManipulator>
    void TryManipulateComponentWithMatchingManipulator(
        ui::Gizmo& gizmo,
        const Rect& screenRect,
        const PolarPerspectiveCamera& camera,
        const std::shared_ptr<UndoableModelStatePair>& model,
        const OpenSim::Component& selected,
        Typelist<ConcreteManipulator...>)
    {
        (TryManipulateComponentWithManipulator<ConcreteManipulator>(gizmo, screenRect, camera, model, selected) || ...);
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

void osc::ModelSelectionGizmo::onDraw(
    const Rect& screenRect,
    const PolarPerspectiveCamera& camera)
{
    const OpenSim::Component* selected = m_Model->getSelected();
    if (not selected) {
        return;
    }

    TryManipulateComponentWithMatchingManipulator(
        m_Gizmo,
        screenRect,
        camera,
        m_Model,
        *selected,
        ManipulatorList{}
    );
}

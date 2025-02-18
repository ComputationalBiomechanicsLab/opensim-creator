#include "ModelSelectionGizmo.h"

#include <libopensimcreator/Documents/Model/IModelStatePair.h>
#include <libopensimcreator/Documents/Model/UndoableModelActions.h>
#include <libopensimcreator/Utils/OpenSimHelpers.h>
#include <libopensimcreator/Utils/SimTKConverters.h>

#include <liboscar/Maths/Angle.h>
#include <liboscar/Maths/EulerAngles.h>
#include <liboscar/Maths/Mat4.h>
#include <liboscar/Maths/MatFunctions.h>
#include <liboscar/Maths/MathHelpers.h>
#include <liboscar/Maths/PolarPerspectiveCamera.h>
#include <liboscar/Maths/Quat.h>
#include <liboscar/Maths/QuaternionFunctions.h>
#include <liboscar/Maths/Rect.h>
#include <liboscar/Maths/RectFunctions.h>
#include <liboscar/Maths/Vec.h>
#include <liboscar/Maths/Vec3.h>
#include <liboscar/Maths/Vec4.h>
#include <liboscar/Maths/VecFunctions.h>
#include <liboscar/Platform/Log.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/Utils/Assertions.h>
#include <liboscar/Utils/ScopeExit.h>
#include <liboscar/Utils/StringHelpers.h>
#include <liboscar/Utils/Typelist.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/ContactGeometry.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PathPoint.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>
#include <OpenSim/Simulation/Wrap/WrapObject.h>

#include <concepts>
#include <cstddef>
#include <memory>
#include <sstream>
#include <type_traits>
#include <utility>

using namespace osc;

// common/virtual manipulator data/APIs
namespace
{
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

        ui::GizmoOperations getSupportedManipulationOps() const
        {
            return implGetSupportedManipulationOps();
        }

        Mat4 getCurrentTransformInGround() const
        {
            return implGetCurrentTransformInGround();
        }

        void onApplyTransform(const SimTK::Transform& transformInGround)
        {
            implOnApplyTransform(transformInGround);
        }

        void onSave()
        {
            implOnSave();
        }

        void drawExtraOnUsingOverlays(
            ui::DrawListView drawList,
            const Mat4& viewMatrix,
            const Mat4& projectionMatrix,
            const Rect& screenRect) const
        {
            implDrawExtraOnUsingOverlays(drawList, viewMatrix, projectionMatrix, screenRect);
        }
    private:
        virtual ui::GizmoOperations implGetSupportedManipulationOps() const = 0;
        virtual Mat4 implGetCurrentTransformInGround() const = 0;
        virtual void implOnApplyTransform(const SimTK::Transform&) = 0;
        virtual void implOnSave() = 0;
        virtual void implDrawExtraOnUsingOverlays(
            ui::DrawListView,
            const Mat4&,
            const Mat4&,
            const Rect&) const
        {}
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
            std::shared_ptr<IModelStatePair> model_,
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

        IModelStatePair& getUndoableModel()
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
        void implOnApplyTransform(const SimTK::Transform& transformInGround) final
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
        virtual void implOnApplyTransform(const AssociatedComponent&, const SimTK::Transform& transformInGround) = 0;

        std::shared_ptr<IModelStatePair> m_Model;
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
            std::shared_ptr<IModelStatePair> model_,
            const OpenSim::Station& station) :
            SelectionManipulator{std::move(model_), station}
        {}

    private:
        ui::GizmoOperations implGetSupportedManipulationOps() const final
        {
            return ui::GizmoOperation::Translate;
        }

        Mat4 implGetCurrentTransformInGround(
            const OpenSim::Station& station) const final
        {
            const SimTK::State& state = getState();
            Mat4 transformInGround = to<Mat4>(station.getParentFrame().getRotationInGround(state));
            transformInGround[3] = Vec4{to<Vec3>(station.getLocationInGround(state)), 1.0f};

            return transformInGround;
        }

        void implOnApplyTransform(
            const OpenSim::Station& station,
            const SimTK::Transform& transformInGround) final
        {
            // ignores `rotation`

            const SimTK::Rotation parentToGroundRotation = station.getParentFrame().getRotationInGround(getState());
            const SimTK::InverseRotation& groundToParentRotation = parentToGroundRotation.invert();
            const Vec3 translationInParent = to<Vec3>(groundToParentRotation * transformInGround.p());

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
            std::shared_ptr<IModelStatePair> model_,
            const OpenSim::PathPoint& pathPoint) :
            SelectionManipulator{std::move(model_), pathPoint}
        {}

    private:
        ui::GizmoOperations implGetSupportedManipulationOps() const final
        {
            return ui::GizmoOperation::Translate;
        }

        Mat4 implGetCurrentTransformInGround(
            const OpenSim::PathPoint& pathPoint) const final
        {
            const SimTK::State& state = getState();
            Mat4 transformInGround = to<Mat4>(pathPoint.getParentFrame().getRotationInGround(state));
            transformInGround[3] = Vec4{to<Vec3>(pathPoint.getLocationInGround(state)), 1.0f};

            return transformInGround;
        }

        void implOnApplyTransform(
            const OpenSim::PathPoint& pathPoint,
            const SimTK::Transform& transformInGround) final
        {
            // ignores `rotation`

            const SimTK::Rotation parentToGroundRotation = pathPoint.getParentFrame().getRotationInGround(getState());
            const SimTK::InverseRotation& groundToParentRotation = parentToGroundRotation.invert();
            const Vec3 translationInParent = to<Vec3>(groundToParentRotation * transformInGround.p());

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

    // an `ISelectionManipulator` that manipulates an `OpenSim::PhysicalOffsetFrame`
    class PhysicalOffsetFrameManipulator final : public SelectionManipulator<OpenSim::PhysicalOffsetFrame> {
    public:
        PhysicalOffsetFrameManipulator(
            std::shared_ptr<IModelStatePair> model_,
            const OpenSim::PhysicalOffsetFrame& pof) :
            SelectionManipulator{std::move(model_), pof},
            m_IsChildFrameOfJoint{IsDirectChildOfAnyJoint(getModel(), pof)}
        {}

    private:
        ui::GizmoOperations implGetSupportedManipulationOps() const final
        {
            return {ui::GizmoOperation::Translate, ui::GizmoOperation::Rotate};
        }

        Mat4 implGetCurrentTransformInGround(
            const OpenSim::PhysicalOffsetFrame& pof) const final
        {
            if (m_IsChildFrameOfJoint) {
                // if the POF that's being edited is the child frame of a joint then
                // its offset/orientation is constrained to be in the same location/orientation
                // as the joint's parent frame (plus coordinate transforms)
                return to<Mat4>(pof.getParentFrame().getTransformInGround(getState()));
            }
            else {
                return to<Mat4>(pof.getTransformInGround(getState()));
            }
        }

        void implOnApplyTransform(
            const OpenSim::PhysicalOffsetFrame& pof,
            const SimTK::Transform& M_n) final
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

                const SimTK::Transform& M_pofg = pof.getTransformInGround(getState());
                const SimTK::Transform M_p = pof.findTransformBetween(getState(), pof.getParentFrame());
                const SimTK::Transform X = (M_pofg.invert() * M_n * M_pofg * M_p.invert()).invert();
                ActionTransformPofV2(
                    getUndoableModel(),
                    pof,
                    to<Vec3>(X.p()),
                    to<EulerAngles>(X.R())
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

                const SimTK::Transform M_g = pof.getParentFrame().getTransformInGround(getState());
                const SimTK::Transform M_p = pof.findTransformBetween(getState(), pof.getParentFrame());
                const SimTK::Transform X = M_g.invert() * M_n * M_g * M_p;
                ActionTransformPofV2(
                    getUndoableModel(),
                    pof,
                    to<Vec3>(X.p()),
                    to<EulerAngles>(X.R())
                );
            }
        }

        void implDrawExtraOnUsingOverlays(
            ui::DrawListView drawList,
            const Mat4& viewMatrix,
            const Mat4& projectionMatrix,
            const Rect& screenRect) const final
        {
            if (not m_IsChildFrameOfJoint) {
                return;  // "normal" offset frames have no additional overlays
            }
            const OpenSim::PhysicalOffsetFrame* pof = findSelection();
            if (not pof) {
                return;  // lookup failed
            }

            // If the user is manipulating a child offset frame, then provide an in-UI
            // annotation that explains that the user isn't actually manipulating the
            // child frame, but its parent, to try and reduce user confusion (#955).

            std::stringstream ss;
            ss << "Note: this is effectively moving " << pof->getParentFrame().getName() << ", because " << pof->getName() << " is\nconstrained by a joint.";
            const std::string label = std::move(ss).str();
            const Vec3 worldPos{getCurrentTransformInGround()[3]};
            const Vec2 screenPos = project_onto_screen_rect(worldPos, viewMatrix, projectionMatrix, screenRect);
            const Vec2 offset = ui::gizmo_annotation_offset() + Vec2{0.0f, ui::get_text_line_height()};

            drawList.add_text(screenPos + offset + 1.0f, Color::black(), label);
            drawList.add_text(screenPos + offset, Color::white(), label);
        }

        bool m_IsChildFrameOfJoint = false;
    };

    // manipulator for `OpenSim::WrapObject`
    class WrapObjectManipulator final : public SelectionManipulator<OpenSim::WrapObject> {
    public:
        WrapObjectManipulator(
            std::shared_ptr<IModelStatePair> model_,
            const OpenSim::WrapObject& wo) :
            SelectionManipulator{std::move(model_), wo}
        {}

    private:
        ui::GizmoOperations implGetSupportedManipulationOps() const final
        {
            return {ui::GizmoOperation::Rotate, ui::GizmoOperation::Translate};
        }

        Mat4 implGetCurrentTransformInGround(
            const OpenSim::WrapObject& wrapObj) const final
        {
            const SimTK::Transform& wrapToFrame = wrapObj.getTransform();
            const SimTK::Transform frameToGround = wrapObj.getFrame().getTransformInGround(getState());
            const SimTK::Transform wrapToGround = frameToGround * wrapToFrame;

            return to<Mat4>(wrapToGround);
        }

        void implOnApplyTransform(
            const OpenSim::WrapObject& wrapObj,
            const SimTK::Transform& M_n) final
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

            const SimTK::Transform M_g = wrapObj.getFrame().getTransformInGround(getState());
            const SimTK::Transform& M_w = wrapObj.getTransform();
            const SimTK::Transform X = M_g.invert() * M_n * M_g * M_w;
            ActionTransformWrapObject(
                getUndoableModel(),
                wrapObj,
                to<Vec3>(X.p() - M_w.p()),
                to<EulerAngles>(X.R())
            );
        }
    };

    // manipulator for `OpenSim::ContactGeometry`
    class ContactGeometryManipulator final : public SelectionManipulator<OpenSim::ContactGeometry> {
    public:
        ContactGeometryManipulator(
            std::shared_ptr<IModelStatePair> model_,
            const OpenSim::ContactGeometry& contactGeom) :
            SelectionManipulator{std::move(model_), contactGeom}
        {}

    private:
        ui::GizmoOperations implGetSupportedManipulationOps() const final
        {
            return {ui::GizmoOperation::Rotate, ui::GizmoOperation::Translate};
        }

        Mat4 implGetCurrentTransformInGround(
            const OpenSim::ContactGeometry& contactGeom) const final
        {
            const SimTK::Transform wrapToFrame = contactGeom.getTransform();
            const SimTK::Transform frameToGround = contactGeom.getFrame().getTransformInGround(getState());
            const SimTK::Transform wrapToGround = frameToGround * wrapToFrame;

            return to<Mat4>(wrapToGround);
        }

        void implOnApplyTransform(
            const OpenSim::ContactGeometry& contactGeom,
            const SimTK::Transform& M_n) final
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

            const SimTK::Transform M_g = contactGeom.getFrame().getTransformInGround(getState());
            const SimTK::Transform M_w = contactGeom.getTransform();
            const SimTK::Transform X = M_g.invert() * M_n * M_g * M_w;
            ActionTransformContactGeometry(
                getUndoableModel(),
                contactGeom,
                to<Vec3>(X.p() - M_w.p()),
                to<EulerAngles>(X.R())
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
            std::shared_ptr<IModelStatePair> model_,
            const OpenSim::Joint& joint) :
            SelectionManipulator{std::move(model_), joint}
        {}

    private:
        ui::GizmoOperations implGetSupportedManipulationOps() const final
        {
            return {ui::GizmoOperation::Translate, ui::GizmoOperation::Rotate};
        }

        Mat4 implGetCurrentTransformInGround(const OpenSim::Joint& joint) const final
        {
            // present the "joint center" as equivalent to the parent frame
            return to<Mat4>(joint.getParentFrame().getTransformInGround(getState()));
        }

        void implOnApplyTransform(const OpenSim::Joint& joint, const SimTK::Transform& M_n) final
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
                const SimTK::Transform M_g = parentPOF.getParentFrame().getTransformInGround(getState());
                const SimTK::Transform X = M_g.invert() * M_n * M_g * M_ppof1;
                ActionTransformPofV2(
                    getUndoableModel(),
                    parentPOF,
                    to<Vec3>(X.p()),
                    to<EulerAngles>(X.R())
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
                to<Vec3>(M_cpof2.p()),
                to<EulerAngles>(M_cpof2.R())
            );
        }

        void implDrawExtraOnUsingOverlays(
            ui::DrawListView drawList,
            const Mat4& viewMatrix,
            const Mat4& projectionMatrix,
            const Rect& screenRect) const final
        {
            const OpenSim::Joint* joint = findSelection();
            if (not joint) {
                return;  // lookup failed
            }

            // If the user is manipulating a joint center then provide an in-UI
            // annotation that highlights the fact that moving a joint center
            // manipulates both the parent and child frames of the joint

            std::stringstream ss;
            ss << "Note: manipulating the joint center moves both the parent (" << joint->getParentFrame().getName() << ") and\nchild (" << joint->getParentFrame().getName() << ") frames.";
            const std::string label = std::move(ss).str();
            const Vec3 worldPos{getCurrentTransformInGround()[3]};
            const Vec2 screenPos = project_onto_screen_rect(worldPos, viewMatrix, projectionMatrix, screenRect);
            const Vec2 offset = ui::gizmo_annotation_offset() + Vec2{0.0f, ui::get_text_line_height()};

            drawList.add_text(screenPos + offset + 1.0f, Color::black(), label);
            drawList.add_text(screenPos + offset, Color::white(), label);
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
        //
        // If the current operation isn't actually supported by the current manipulator, but
        // the current manipulator supports something else, then the gizmo should coerce its
        // current operation to a supported one. This is to handle the case where (e.g.) a
        // user is manipulating something that's rotate-able but then select something that's
        // only translate-able (#705)
        {
            const ui::GizmoOperations supportedOperations = manipulator.getSupportedManipulationOps();
            const ui::GizmoOperation currentOperation = gizmo.operation();

            if (not supportedOperations) {
                return;  // no operations are supported by the manipulator at all
            }

            if (!(currentOperation & supportedOperations)) {
                // the manipulator supports _something_, but it isn't the same as the current
                // operation, so we coerce the current operation to something that's supported
                gizmo.set_operation(supportedOperations.lowest_set());
            }
        }

        // draw the manipulator
        Mat4 modelMatrix = manipulator.getCurrentTransformInGround();
        const Mat4 viewMatrix = camera.view_matrix();
        const Mat4 projectionMatrix = camera.projection_matrix(aspect_ratio_of(screenRect));

        const auto userEditInGround = gizmo.draw(modelMatrix, viewMatrix, projectionMatrix, screenRect);

        if (gizmo.is_using()) {  // note: using != manipulating
            // draw any additional annotations over the top
            manipulator.drawExtraOnUsingOverlays(ui::get_panel_draw_list(), viewMatrix, projectionMatrix, screenRect);
        }

        if (userEditInGround) {
            // propagate user edit to the model via the `ISelectionManipulator` interface
            manipulator.onApplyTransform(SimTK::Transform{to<SimTK::Rotation>(userEditInGround->rotation), to<SimTK::Vec3>(userEditInGround->position)});
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
        const std::shared_ptr<IModelStatePair>& model,
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
        const std::shared_ptr<IModelStatePair>& model,
        const OpenSim::Component& selected,
        Typelist<ConcreteManipulator...>)
    {
        (TryManipulateComponentWithManipulator<ConcreteManipulator>(gizmo, screenRect, camera, model, selected) || ...);
    }
}

osc::ModelSelectionGizmo::ModelSelectionGizmo(std::shared_ptr<IModelStatePair> model_) :
    m_Model{std::move(model_)}
{
    // Default the gizmo to local-space, because OpenSim users can confuse the
    // gizmo arrows with the frame they've currently selected. If the frames are
    // hidden in the UI view, then they'll think "oh, these gizmo arrows are all
    // wrong, my frame is rotated!", even though the gizmo isn't the frame (#928).
    m_Gizmo.set_mode(ui::GizmoMode::Local);
}
osc::ModelSelectionGizmo::ModelSelectionGizmo(const ModelSelectionGizmo&) = default;
osc::ModelSelectionGizmo::ModelSelectionGizmo(ModelSelectionGizmo&&) noexcept = default;
osc::ModelSelectionGizmo& osc::ModelSelectionGizmo::operator=(const ModelSelectionGizmo&) = default;
osc::ModelSelectionGizmo& osc::ModelSelectionGizmo::operator=(ModelSelectionGizmo&&) noexcept = default;
osc::ModelSelectionGizmo::~ModelSelectionGizmo() noexcept = default;

void osc::ModelSelectionGizmo::onDraw(
    const Rect& screenRect,
    const PolarPerspectiveCamera& camera)
{
    if (m_Model->isReadonly()) {
        return;  // cannot manipulate a readonly model (#936)
    }

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

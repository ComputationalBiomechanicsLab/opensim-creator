#include "model_selection_gizmo.h"

#include <libopensimcreator/documents/model/undoable_model_actions.h>

#include <libopynsim/documents/model/model_state_pair.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <libopynsim/utilities/simbody_x_oscar.h>
#include <liboscar/graphics/camera.h>
#include <liboscar/maths/euler_angles.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/matrix4x4.h>
#include <liboscar/maths/quaternion.h>
#include <liboscar/maths/rect.h>
#include <liboscar/maths/rect_functions.h>
#include <liboscar/maths/vector.h>
#include <liboscar/platform/log.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/utilities/assertions.h>
#include <liboscar/utilities/string_helpers.h>
#include <liboscar/utilities/typelist.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/ContactGeometry.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PathPoint.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <OpenSim/Simulation/Model/Scholz2015GeometryPath.h>
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
    // Abstract interface to an object that manipulates something in a model.
    class ISelectionManipulator {
    protected:
        ISelectionManipulator() = default;
        ISelectionManipulator(const ISelectionManipulator&) = default;
        ISelectionManipulator(ISelectionManipulator&&) noexcept = default;
        ISelectionManipulator& operator=(const ISelectionManipulator&) = default;
        ISelectionManipulator& operator=(ISelectionManipulator&&) noexcept = default;

    public:
        virtual ~ISelectionManipulator() noexcept = default;

        ui::GizmoOperations get_supported_manipulation_ops() const
        {
            return impl_get_supported_manipulation_ops();
        }
        Matrix4x4 get_current_transform_in_ground() const
        {
            return impl_get_current_transform_in_ground();
        }
        void on_apply_transform(const SimTK::Transform& transform_in_ground)
        {
            impl_on_apply_transform(transform_in_ground);
        }
        void on_save() { impl_on_save(); }

        void draw_extra_on_using_overlays(
            ui::DrawListView draw_list,
            const Matrix4x4& view_matrix,
            const Matrix4x4& projection_matrix,
            const Rect& screen_rect) const
        {
            impl_draw_extra_on_using_overlays(
                std::move(draw_list), view_matrix, projection_matrix, screen_rect);
        }

    private:
        // Implementors must return which operations are supported by this `ISelectionManipulator`.
        virtual ui::GizmoOperations impl_get_supported_manipulation_ops() const = 0;

        // Implementors must return a transform matrix that describes the edited transform with
        // respect to ground.
        virtual Matrix4x4 impl_get_current_transform_in_ground() const = 0;

        // Implementors must apply the provided delta transform (i.e. the _change_ made by the user)
        // to the appropriate component.
        virtual void impl_on_apply_transform(const SimTK::Transform& delta_transform) = 0;

        // Implementors must commit/finalize any edits to the model.
        virtual void impl_on_save() = 0;

        // Implementors may override this to draw extra overlays in the 2D UI during manipulation
        virtual void impl_draw_extra_on_using_overlays(
            ui::DrawListView, const Matrix4x4&, const Matrix4x4&, const Rect&) const
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
            std::shared_ptr<opyn::ModelStatePair> model, const AssociatedComponent& component) :

            model_{std::move(model)},
            component_abs_path_{component.getAbsolutePath()}
        {
            OSC_ASSERT(model_ != nullptr);
            OSC_ASSERT(
                opyn::find_component<AssociatedComponent>(
                    model_->get_model(), component_abs_path_));
        }

        const AssociatedComponent* find_selection() const
        {
            return opyn::find_component<AssociatedComponent>(
                model_->get_model(), component_abs_path_);
        }

        const OpenSim::Model& get_model() const { return model_->get_model(); }

        const SimTK::State& get_state() const { return model_->get_state(); }

        opyn::ModelStatePair& get_undoable_model() { return *model_; }

    private:
        // perform runtime lookup for `AssociatedComponent` and forward into concrete implementation
        Matrix4x4 impl_get_current_transform_in_ground() const override
        {
            const AssociatedComponent* maybe_selected = find_selection();
            if (not maybe_selected) {
                return identity<Matrix4x4>();  // selection of that type does not exist in the model
            }
            return impl_get_current_transform_in_ground(*maybe_selected);
        }

        // perform runtime lookup for `AssociatedComponent` and forward into concrete implementation
        void impl_on_apply_transform(const SimTK::Transform& transform_in_ground) override
        {
            const AssociatedComponent* maybe_selected = find_selection();
            if (not maybe_selected) {
                return;  // selection of that type does not exist in the model
            }
            impl_on_apply_transform(*maybe_selected, transform_in_ground);
        }

        void impl_on_save() override
        {
            const AssociatedComponent* maybe_selected = find_selection();
            if (not maybe_selected) {
                return;  // selection of that type does not exist in the model
            }
            impl_on_save(*maybe_selected);
        }

        virtual void impl_on_save(const AssociatedComponent& component)
        {
            std::stringstream ss;
            ss << "transformed " << component.getName();
            get_undoable_model().commit(std::move(ss).str());
        }

        // inheritors must implement concrete manipulation methods
        virtual Matrix4x4 impl_get_current_transform_in_ground(
            const AssociatedComponent&) const = 0;
        virtual void impl_on_apply_transform(
            const AssociatedComponent&, const SimTK::Transform& transform_in_ground) = 0;

        std::shared_ptr<opyn::ModelStatePair> model_;
        OpenSim::ComponentPath component_abs_path_;
    };
}

// concrete manipulator implementations
namespace
{
    // an `ISelectionManipulator` that manipulates an `OpenSim::Station`
    class StationManipulator final : public SelectionManipulator<OpenSim::Station> {
    public:
        StationManipulator(
            std::shared_ptr<opyn::ModelStatePair> model, const OpenSim::Station& station) :

            SelectionManipulator{std::move(model), station}
        {}

    private:
        ui::GizmoOperations impl_get_supported_manipulation_ops() const override
        {
            return ui::GizmoOperation::Translate;
        }

        Matrix4x4 impl_get_current_transform_in_ground(
            const OpenSim::Station& station) const override
        {
            const SimTK::State& state = get_state();
            auto transform_in_ground =
                to<Matrix4x4>(station.getParentFrame().getRotationInGround(state));
            transform_in_ground[3] = Vector4{to<Vector3>(station.getLocationInGround(state)), 1.0f};

            return transform_in_ground;
        }

        void impl_on_apply_transform(
            const OpenSim::Station& station, const SimTK::Transform& transform_in_ground) override
        {
            // ignores `rotation`

            const SimTK::Rotation parent_to_ground_rotation =
                station.getParentFrame().getRotationInGround(get_state());
            const SimTK::InverseRotation& ground_to_parent_rotation =
                parent_to_ground_rotation.invert();
            const auto translation_in_parent =
                to<Vector3>(ground_to_parent_rotation * transform_in_ground.p());

            action_translate_station(get_undoable_model(), station, translation_in_parent);
        }

        void impl_on_save(const OpenSim::Station& station) override
        {
            action_translate_station_and_save(get_undoable_model(), station, {});
        }
    };

    // an `ISelectionManpulator` that manipulates an `OpenSim::PathPoint`
    class PathPointManipulator : public SelectionManipulator<OpenSim::PathPoint> {
    public:
        PathPointManipulator(
            std::shared_ptr<opyn::ModelStatePair> model, const OpenSim::PathPoint& path_point) :

            SelectionManipulator{std::move(model), path_point}
        {}

    private:
        ui::GizmoOperations impl_get_supported_manipulation_ops() const override
        {
            return ui::GizmoOperation::Translate;
        }

        Matrix4x4 impl_get_current_transform_in_ground(
            const OpenSim::PathPoint& path_point) const override
        {
            const SimTK::State& state = get_state();
            auto transform_in_ground =
                to<Matrix4x4>(path_point.getParentFrame().getRotationInGround(state));
            transform_in_ground[3] =
                Vector4{to<Vector3>(path_point.getLocationInGround(state)), 1.0f};

            return transform_in_ground;
        }

        void impl_on_apply_transform(
            const OpenSim::PathPoint& path_point,
            const SimTK::Transform& transform_in_ground) override
        {
            // ignores `rotation`

            const SimTK::Rotation parent_to_ground_rotation =
                path_point.getParentFrame().getRotationInGround(get_state());
            const SimTK::InverseRotation& ground_to_parent_rotation =
                parent_to_ground_rotation.invert();
            const auto translation_in_parent =
                to<Vector3>(ground_to_parent_rotation * transform_in_ground.p());

            action_translate_path_point(get_undoable_model(), path_point, translation_in_parent);
        }

        void impl_on_save(const OpenSim::PathPoint& path_point) override
        {
            action_translate_path_point_and_save(get_undoable_model(), path_point, {});
        }
    };

    bool is_direct_child_of_any_joint(const OpenSim::Model& model, const OpenSim::Frame& frame)
    {
        for (const OpenSim::Joint& joint : model.getComponentList<OpenSim::Joint>()) {
            if (&joint.getChildFrame() == &frame) {
                return true;
            }
        }
        return false;
    }

    // an `ISelectionManipulator` that manipulates an `OpenSim::PhysicalOffsetFrame`
    class PhysicalOffsetFrameManipulator final
        : public SelectionManipulator<OpenSim::PhysicalOffsetFrame> {
    public:
        PhysicalOffsetFrameManipulator(
            std::shared_ptr<opyn::ModelStatePair> model, const OpenSim::PhysicalOffsetFrame& pof) :

            SelectionManipulator{std::move(model), pof},
            is_child_frame_of_joint_{is_direct_child_of_any_joint(get_model(), pof)}
        {}

    private:
        ui::GizmoOperations impl_get_supported_manipulation_ops() const override
        {
            return {ui::GizmoOperation::Translate, ui::GizmoOperation::Rotate};
        }

        Matrix4x4 impl_get_current_transform_in_ground(
            const OpenSim::PhysicalOffsetFrame& pof) const override
        {
            if (is_child_frame_of_joint_) {
                // if the POF that's being edited is the child frame of a joint then
                // its offset/orientation is constrained to be in the same location/orientation
                // as the joint's parent frame (plus coordinate transforms)
                return to<Matrix4x4>(pof.getParentFrame().getTransformInGround(get_state()));
            } else {
                return to<Matrix4x4>(pof.getTransformInGround(get_state()));
            }
        }

        void impl_on_apply_transform(
            const OpenSim::PhysicalOffsetFrame& pof, const SimTK::Transform& m_n) override
        {
            if (is_child_frame_of_joint_) {
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

                const SimTK::Transform& m_pofg = pof.getTransformInGround(get_state());
                const SimTK::Transform m_p =
                    pof.findTransformBetween(get_state(), pof.getParentFrame());
                const SimTK::Transform x = (m_pofg.invert() * m_n * m_pofg * m_p.invert()).invert();
                action_transform_pof_v2(
                    get_undoable_model(), pof, to<Vector3>(x.p()), to<EulerAngles>(x.R()));
            } else {
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

                const SimTK::Transform m_g = pof.getParentFrame().getTransformInGround(get_state());
                const SimTK::Transform m_p =
                    pof.findTransformBetween(get_state(), pof.getParentFrame());
                const SimTK::Transform x = m_g.invert() * m_n * m_g * m_p;
                action_transform_pof_v2(
                    get_undoable_model(), pof, to<Vector3>(x.p()), to<EulerAngles>(x.R()));
            }
        }

        void impl_draw_extra_on_using_overlays(
            ui::DrawListView draw_list,
            const Matrix4x4& view_matrix,
            const Matrix4x4& projection_matrix,
            const Rect& screen_rect) const override
        {
            if (not is_child_frame_of_joint_) {
                return;  // "normal" offset frames have no additional overlays
            }
            const OpenSim::PhysicalOffsetFrame* pof = find_selection();
            if (not pof) {
                return;  // lookup failed
            }

            // If the user is manipulating a child offset frame, then provide an in-UI
            // annotation that explains that the user isn't actually manipulating the
            // child frame, but its parent, to try and reduce user confusion (#955).

            std::stringstream ss;
            ss << "Note: this is effectively moving " << pof->getParentFrame().getName()
               << ", because " << pof->getName() << " is\nconstrained by a joint.";
            const std::string label = std::move(ss).str();
            const Vector3 world_pos{get_current_transform_in_ground()[3]};
            const Vector2 screen_pos =
                project_onto_viewport_rect(world_pos, view_matrix, projection_matrix, screen_rect);
            const Vector2 offset = ui::gizmo_annotation_offset() +
                                   Vector2{0.0f, ui::get_text_line_height_in_current_panel()};

            draw_list.add_text(screen_pos + offset + 1.0f, Color::black(), label);
            draw_list.add_text(screen_pos + offset, Color::white(), label);
        }

        bool is_child_frame_of_joint_ = false;
    };

    // manipulator for `OpenSim::WrapObject`
    class WrapObjectManipulator final : public SelectionManipulator<OpenSim::WrapObject> {
    public:
        WrapObjectManipulator(
            std::shared_ptr<opyn::ModelStatePair> model, const OpenSim::WrapObject& wo) :

            SelectionManipulator{std::move(model), wo}
        {}

    private:
        ui::GizmoOperations impl_get_supported_manipulation_ops() const override
        {
            return {ui::GizmoOperation::Rotate, ui::GizmoOperation::Translate};
        }

        Matrix4x4 impl_get_current_transform_in_ground(
            const OpenSim::WrapObject& wrap_obj) const override
        {
            const SimTK::Transform& wrap_to_frame = wrap_obj.getTransform();
            const SimTK::Transform frame_to_ground =
                wrap_obj.getFrame().getTransformInGround(get_state());
            const SimTK::Transform wrap_to_ground = frame_to_ground * wrap_to_frame;

            return to<Matrix4x4>(wrap_to_ground);
        }

        void impl_on_apply_transform(
            const OpenSim::WrapObject& wrap_obj, const SimTK::Transform& m_n) override
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

            const SimTK::Transform m_g = wrap_obj.getFrame().getTransformInGround(get_state());
            const SimTK::Transform& m_w = wrap_obj.getTransform();
            const SimTK::Transform x = m_g.invert() * m_n * m_g * m_w;
            action_transform_wrap_object(
                get_undoable_model(),
                wrap_obj,
                to<Vector3>(x.p() - m_w.p()),
                to<EulerAngles>(x.R()));
        }
    };

    // manipulator for `OpenSim::ContactGeometry`
    class ContactGeometryManipulator final : public SelectionManipulator<OpenSim::ContactGeometry> {
    public:
        ContactGeometryManipulator(
            std::shared_ptr<opyn::ModelStatePair> model,
            const OpenSim::ContactGeometry& contact_geom) :
            SelectionManipulator{std::move(model), contact_geom}
        {}

    private:
        ui::GizmoOperations impl_get_supported_manipulation_ops() const override
        {
            return {ui::GizmoOperation::Rotate, ui::GizmoOperation::Translate};
        }

        Matrix4x4 impl_get_current_transform_in_ground(
            const OpenSim::ContactGeometry& contact_geom) const override
        {
            const SimTK::Transform wrap_to_frame = contact_geom.getTransform();
            const SimTK::Transform frame_to_ground =
                contact_geom.getFrame().getTransformInGround(get_state());
            const SimTK::Transform wrap_to_ground = frame_to_ground * wrap_to_frame;

            return to<Matrix4x4>(wrap_to_ground);
        }

        void impl_on_apply_transform(
            const OpenSim::ContactGeometry& contact_geom, const SimTK::Transform& m_n) override
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

            const SimTK::Transform m_g = contact_geom.getFrame().getTransformInGround(get_state());
            const SimTK::Transform m_w = contact_geom.getTransform();
            const SimTK::Transform x = m_g.invert() * m_n * m_g * m_w;
            action_transform_contact_geometry(
                get_undoable_model(),
                contact_geom,
                to<Vector3>(x.p() - m_w.p()),
                to<EulerAngles>(x.R()));
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

        JointManipulator(std::shared_ptr<opyn::ModelStatePair> model, const OpenSim::Joint& joint) :
            SelectionManipulator{std::move(model), joint}
        {}

    private:
        ui::GizmoOperations impl_get_supported_manipulation_ops() const override
        {
            return {ui::GizmoOperation::Translate, ui::GizmoOperation::Rotate};
        }

        Matrix4x4 impl_get_current_transform_in_ground(const OpenSim::Joint& joint) const override
        {
            // present the "joint center" as equivalent to the parent frame
            return to<Matrix4x4>(joint.getParentFrame().getTransformInGround(get_state()));
        }

        void impl_on_apply_transform(
            const OpenSim::Joint& joint, const SimTK::Transform& m_n) override
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
            // - M_cpof1                    joint child offset frame to its parent transform
            //                              (1: BEFORE)
            // - M_j                        joint child-to-parent transform
            // - M_ppof1                    joint parent offset frame to its parent transform
            //                              (1: BEFORE)
            // - M_ppof2                    joint parent offset frame to its parent transform
            //                              (2: AFTER)
            // - M_cpof2  **WE WANT THIS**  joint child offset frame to its parent transform
            //                              (2: AFTER)
            // - vcp                        an example quantity, expressed in the child's parent
            //                              frame (e.g. a body)
            // - vjp                        the same example quantity, but expressed in the joint's
            //                              parent frame
            //
            // computing `vjp` from `vcp` involves going up the kinematic chain:
            //
            //     vjp = M_ppof1 * M_j * M_cpof1^-1 * vcp
            //
            // now, our goal is to apply STEP 1 (M_ppof1 --> M_ppof2) and calculate a new `M_cpof2`
            // such that quantities expressed in a child body (e.g. `vcp`) do not move in the scene.
            // I.e.:
            //
            //     vjp = M_ppof1 * M_j * M_cpof1^-1 * vcp = M_ppof2 * M_j * M_cpof2^-1 * vcp
            //
            // simplifying, and dropping the pretext of using the transforms to transform a
            // particular point:
            //
            //     M_ppof1 * M_j * M_cpof1^-1 = M_ppof2 * M_j * M_cpof2^-1
            //     M_ppof2^-1 * M_ppof1 * M_j * M_cpof1^-1 = M_j * M_cpof2^-1
            //     M_j^-1 * M_ppof2^-1 * M_ppof1 * M_j * M_cpof1^-1 = M_cpof2^-1
            //     M_cpof2^-1 = M_j^-1 * M_ppof2^-1 * M_ppof1 * M_j * M_cpof1^-1
            //     M_cpof2 = (M_j^-1 * M_ppof2^-1 * M_ppof1 * M_j * M_cpof1^-1)^-1;
            //
            // the code below essentially collects all of this information up to figure out
            // `M_cpof2` and stuff it into the child `OpenSim::PhysicalOffsetFrame`

            const auto& parent_pof =
                dynamic_cast<const OpenSim::PhysicalOffsetFrame&>(joint.getParentFrame());
            const auto& child_pof =
                dynamic_cast<const OpenSim::PhysicalOffsetFrame&>(joint.getChildFrame());

            // get BEFORE transforms
            const SimTK::Transform m_j = child_pof.findTransformBetween(get_state(), parent_pof);
            const SimTK::Transform m_ppof1 =
                parent_pof.findTransformBetween(get_state(), parent_pof.getParentFrame());
            const SimTK::Transform m_cpof1 =
                child_pof.findTransformBetween(get_state(), child_pof.getParentFrame());

            // STEP 1) move the parent offset frame (as normal)
            {
                // M_n * M_g * M_ppof1 * v = M_g * X * v
                const SimTK::Transform m_g =
                    parent_pof.getParentFrame().getTransformInGround(get_state());
                const SimTK::Transform x = m_g.invert() * m_n * m_g * m_ppof1;
                action_transform_pof_v2(
                    get_undoable_model(), parent_pof, to<Vector3>(x.p()), to<EulerAngles>(x.R()));
            }

            // STEP 2) figure out what transform the child offset frame would need to have in
            //         order for its parent (confusing, eh) to not move

            // get AFTER transforms
            const SimTK::Transform m_ppof2 =
                parent_pof.findTransformBetween(get_state(), parent_pof.getParentFrame());

            // caclulate `M_cpof2` (i.e. the desired new child transform)
            const SimTK::Transform m_cpof2 =
                (m_j.invert() * m_ppof2.invert() * m_ppof1 * m_j * m_cpof1.invert()).invert();

            // decompse `M_cpof2` into the child `OpenSim::PhysicalOffsetFrame`'s properties
            action_transform_pof_v2(
                get_undoable_model(),
                child_pof,
                to<Vector3>(m_cpof2.p()),
                to<EulerAngles>(m_cpof2.R()));
        }

        void impl_draw_extra_on_using_overlays(
            ui::DrawListView draw_list,
            const Matrix4x4& view_matrix,
            const Matrix4x4& projection_matrix,
            const Rect& screen_rect) const override
        {
            const OpenSim::Joint* joint = find_selection();
            if (not joint) {
                return;  // lookup failed
            }

            // If the user is manipulating a joint center then provide an in-UI
            // annotation that highlights the fact that moving a joint center
            // manipulates both the parent and child frames of the joint

            std::stringstream ss;
            ss << "Note: manipulating the joint center moves both the parent ("
               << joint->getParentFrame().getName() << ") and\nchild ("
               << joint->getParentFrame().getName() << ") frames.";
            const std::string label = std::move(ss).str();
            const Vector3 world_pos{get_current_transform_in_ground()[3]};
            const Vector2 screen_pos =
                project_onto_viewport_rect(world_pos, view_matrix, projection_matrix, screen_rect);
            const Vector2 offset = ui::gizmo_annotation_offset() +
                                   Vector2{0.0f, ui::get_text_line_height_in_current_panel()};

            draw_list.add_text(screen_pos + offset + 1.0f, Color::black(), label);
            draw_list.add_text(screen_pos + offset, Color::white(), label);
        }
    };

    // an `ISelectionManipulator` that manipulates an `OpenSim::Scholz2015GeometryPathObstacle`
    class Scholz2015GeometryPathObstacleManipulator final
        : public SelectionManipulator<OpenSim::Scholz2015GeometryPathObstacle> {
    public:
        Scholz2015GeometryPathObstacleManipulator(
            std::shared_ptr<opyn::ModelStatePair> model,
            const OpenSim::Scholz2015GeometryPathObstacle& obstacle) :
            SelectionManipulator{std::move(model), obstacle}
        {}

    private:
        ui::GizmoOperations impl_get_supported_manipulation_ops() const override
        {
            return ui::GizmoOperation::Translate;
        }

        Matrix4x4 impl_get_current_transform_in_ground(
            const OpenSim::Scholz2015GeometryPathObstacle& obstacle) const override
        {
            const OpenSim::ContactGeometry& geom = obstacle.getContactGeometry();
            const SimTK::Rotation rotation_in_ground =
                geom.getFrame().getTransformInGround(get_state()).R() * geom.getTransform().R();
            const SimTK::Vec3 translation_in_ground =
                geom.getFrame().getTransformInGround(get_state()) * geom.getTransform() *
                obstacle.getContactHint();
            return matrix4x4_cast(
                {.rotation = to<Quaternion>(rotation_in_ground),
                 .translation = to<Vector3>(translation_in_ground)});
        }

        void impl_on_apply_transform(
            const OpenSim::Scholz2015GeometryPathObstacle& obstacle,
            const SimTK::Transform& transform_in_ground) override
        {
            // ignores `rotation`
            const OpenSim::ContactGeometry& geom = obstacle.getContactGeometry();
            const SimTK::Vec3 translation_in_contact_geom =
                (geom.getFrame().getTransformInGround(get_state()) * geom.getTransform())
                    .R()
                    .invert() *
                transform_in_ground.p();
            action_translate_contact_hint(
                get_undoable_model(), obstacle, to<Vector3>(translation_in_contact_geom));
        }

        void impl_on_save(
            [[maybe_unused]] const OpenSim::Scholz2015GeometryPathObstacle& obstacle) override
        {
            action_translate_contact_hint_and_save(get_undoable_model(), obstacle, {});
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
        JointManipulator,
        Scholz2015GeometryPathObstacleManipulator>;
}

// drawing/rendering code
namespace
{
    // draws the gizmo overlay using the given `ISelectionManipulator`
    void draw_gizmo_overlay_inner(
        ui::Gizmo& gizmo,
        const Rect& screen_rect,
        const Camera& camera,
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
            const ui::GizmoOperations supported_operations =
                manipulator.get_supported_manipulation_ops();
            const ui::GizmoOperation current_operation = gizmo.operation();

            if (not supported_operations) {
                return;  // no operations are supported by the manipulator at all
            }

            if (!(current_operation & supported_operations)) {
                // the manipulator supports _something_, but it isn't the same as the current
                // operation, so we coerce the current operation to something that's supported
                gizmo.set_operation(supported_operations.lowest_set());
            }
        }

        // draw the manipulator
        Matrix4x4 model_matrix = manipulator.get_current_transform_in_ground();
        const Matrix4x4 view_matrix = camera.view_matrix();
        const Matrix4x4 projection_matrix = camera.projection_matrix(aspect_ratio_of(screen_rect));

        const auto user_edit_in_ground =
            gizmo.draw(model_matrix, view_matrix, projection_matrix, screen_rect);

        if (gizmo.is_using()) {  // note: using != manipulating
            // draw any additional annotations over the top
            manipulator.draw_extra_on_using_overlays(
                ui::get_panel_draw_list(), view_matrix, projection_matrix, screen_rect);
        }

        if (user_edit_in_ground) {
            // propagate user edit to the model via the `ISelectionManipulator` interface
            manipulator.on_apply_transform(
                SimTK::Transform{
                    to<SimTK::Rotation>(user_edit_in_ground->rotation),
                    to<SimTK::Vec3>(user_edit_in_ground->translation)});
        }

        // once the user stops using the manipulator, save the changes
        if (gizmo.was_using() and not gizmo.is_using()) {
            manipulator.on_save();
        }
    }

    template<std::derived_from<ISelectionManipulator> ConcreteManipulator>
    bool try_manipulate_component_with_manipulator(
        ui::Gizmo& gizmo,
        const Rect& screen_rect,
        const Camera& camera,
        const std::shared_ptr<opyn::ModelStatePair>& model,
        const OpenSim::Component& selected)
    {
        if (not ConcreteManipulator::matches(selected)) {
            return false;
        }

        const auto* downcasted =
            dynamic_cast<const typename ConcreteManipulator::AssociatedComponent*>(&selected);
        if (not downcasted) {
            return false;
        }

        ConcreteManipulator manipulator{model, *downcasted};
        draw_gizmo_overlay_inner(gizmo, screen_rect, camera, manipulator);
        return true;
    }

    template<typename... ConcreteManipulator>
    void try_manipulate_component_with_matching_manipulator(
        ui::Gizmo& gizmo,
        const Rect& screen_rect,
        const Camera& camera,
        const std::shared_ptr<opyn::ModelStatePair>& model,
        const OpenSim::Component& selected,
        Typelist<ConcreteManipulator...>)
    {
        (try_manipulate_component_with_manipulator<ConcreteManipulator>(
             gizmo, screen_rect, camera, model, selected) ||
         ...);
    }
}

osc::ModelSelectionGizmo::ModelSelectionGizmo(std::shared_ptr<opyn::ModelStatePair> model) :
    model_{std::move(model)}
{
    // Default the gizmo to local-space, because OpenSim users can confuse the
    // gizmo arrows with the frame they've currently selected. If the frames are
    // hidden in the UI view, then they'll think "oh, these gizmo arrows are all
    // wrong, my frame is rotated!", even though the gizmo isn't the frame (#928).
    gizmo_.set_mode(ui::GizmoMode::Local);
}
osc::ModelSelectionGizmo::ModelSelectionGizmo(const ModelSelectionGizmo&) = default;
osc::ModelSelectionGizmo::ModelSelectionGizmo(ModelSelectionGizmo&&) noexcept = default;
osc::ModelSelectionGizmo& osc::ModelSelectionGizmo::operator=(const ModelSelectionGizmo&) = default;
osc::ModelSelectionGizmo& osc::ModelSelectionGizmo::operator=(ModelSelectionGizmo&&) noexcept =
    default;
osc::ModelSelectionGizmo::~ModelSelectionGizmo() noexcept = default;

void osc::ModelSelectionGizmo::on_draw(const Rect& screen_rect, const Camera& camera)
{
    if (model_->is_readonly()) {
        return;  // cannot manipulate a readonly model (#936)
    }

    const OpenSim::Component* selected = model_->get_selected();
    if (not selected) {
        return;
    }

    try_manipulate_component_with_matching_manipulator(
        gizmo_, screen_rect, camera, model_, *selected, ManipulatorList{});
}

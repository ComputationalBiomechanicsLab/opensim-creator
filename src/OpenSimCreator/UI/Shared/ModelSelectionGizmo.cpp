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

    // type-erased virtual base class that each concrete manipulator inherits from
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

        Mat4 getCurrentModelMatrix() const
        {
            return implGetCurrentModelMatrix();
        }

        void onApplyTranslation(const Vec3& deltaTranslationInGround)
        {
            implOnApplyTranslation(deltaTranslationInGround);
        }

        void onApplyRotation(const Eulers& deltaEulerRadiansInGround)
        {
            implOnApplyRotation(deltaEulerRadiansInGround);
        }

        void onSave()
        {
            implOnSave();
        }
    private:
        virtual SupportedManipulationOpFlags implGetSupportedManipulationOps() const = 0;
        virtual Mat4 implGetCurrentModelMatrix() const = 0;
        virtual void implOnApplyTranslation(const Vec3&) {}  // default to noop
        virtual void implOnApplyRotation(const Eulers&) {}  // default to noop
        virtual void implOnSave() = 0;
    };

    // concrete implementation of a selection manipulator for `TComponent`
    //
    // effectively, only stores the model+path to the thing being manipulated, and performs
    // runtime checks to ensure the component still exists in the model
    template<std::derived_from<OpenSim::Component> TComponent>
    class SelectionManipulator : public ISelectionManipulator {
    protected:
        SelectionManipulator(
            std::shared_ptr<UndoableModelStatePair> model_,
            const TComponent& component) :

            m_Model{std::move(model_)},
            m_ComponentAbsPath{component.getAbsolutePath()}
        {
            OSC_ASSERT(m_Model != nullptr);
            OSC_ASSERT(FindComponent<TComponent>(m_Model->getModel(), m_ComponentAbsPath));
        }

        TComponent const* findSelection() const
        {
            return FindComponent<TComponent>(m_Model->getModel(), m_ComponentAbsPath);
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
        // perform runtime lookup for `TComponent` and forward into concrete implementation
        Mat4 implGetCurrentModelMatrix() const final
        {
            TComponent const* maybeSelected = findSelection();
            if (!maybeSelected)
            {
                return identity<Mat4>();  // selection of that type does not exist in the model
            }
            return implGetCurrentModelMatrix(*maybeSelected);
        }

        // perform runtime lookup for `TComponent` and forward into concrete implementation
        void implOnApplyTranslation(const Vec3& deltaTranslationInGround) final
        {
            TComponent const* maybeSelected = findSelection();
            if (!maybeSelected)
            {
                return;  // selection of that type does not exist in the model
            }
            implOnApplyTranslation(*maybeSelected, deltaTranslationInGround);
        }

        // perform runtime lookup for `TComponent` and forward into concrete implementation
        void implOnApplyRotation(const Eulers& deltaEulerRadiansInGround) final
        {
            TComponent const* maybeSelected = findSelection();
            if (!maybeSelected)
            {
                return;  // selection of that type does not exist in the model
            }
            implOnApplyRotation(*maybeSelected, deltaEulerRadiansInGround);
        }

        void implOnSave() final
        {
            TComponent const* maybeSelected = findSelection();
            if (!maybeSelected)
            {
                return;  // selection of that type does not exist in the model
            }
            implOnSave(*maybeSelected);
        }

        // inheritors must implement concrete manipulation methods
        virtual Mat4 implGetCurrentModelMatrix(const TComponent&) const = 0;
        virtual void implOnApplyTranslation(const TComponent&, const Vec3& deltaTranslationInGround) = 0;
        virtual void implOnApplyRotation(const TComponent&, const Eulers&) {}  // default to noop
        virtual void implOnSave(const TComponent&) = 0;

        std::shared_ptr<UndoableModelStatePair> m_Model;
        OpenSim::ComponentPath m_ComponentAbsPath;
    };
}

// concrete manipulator implementations
namespace
{
    // manipulator for `OpenSim::Station`
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

        Mat4 implGetCurrentModelMatrix(
            const OpenSim::Station& station) const final
        {
            const SimTK::State& state = getState();
            Mat4 currentXformInGround = mat4_cast(station.getParentFrame().getRotationInGround(state));
            currentXformInGround[3] = Vec4{ToVec3(station.getLocationInGround(state)), 1.0f};

            return currentXformInGround;
        }

        void implOnApplyTranslation(
            const OpenSim::Station& station,
            const Vec3& deltaTranslationInGround) final
        {
            SimTK::Rotation const parentToGroundRotation = station.getParentFrame().getRotationInGround(getState());
            const SimTK::InverseRotation& groundToParentRotation = parentToGroundRotation.invert();
            Vec3 const translationInParent = ToVec3(groundToParentRotation * ToSimTKVec3(deltaTranslationInGround));

            ActionTranslateStation(getUndoableModel(), station, translationInParent);
        }

        void implOnSave(const OpenSim::Station& station) final
        {
            ActionTranslateStationAndSave(getUndoableModel(), station, {});
        }
    };

    // manipulator for `OpenSim::PathPoint`
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

        Mat4 implGetCurrentModelMatrix(
            const OpenSim::PathPoint& pathPoint) const final
        {
            const SimTK::State& state = getState();
            Mat4 currentXformInGround = mat4_cast(pathPoint.getParentFrame().getRotationInGround(state));
            currentXformInGround[3] = Vec4{ToVec3(pathPoint.getLocationInGround(state)), 1.0f};

            return currentXformInGround;
        }

        void implOnApplyTranslation(
            const OpenSim::PathPoint& pathPoint,
            const Vec3& deltaTranslationInGround) final
        {
            SimTK::Rotation const parentToGroundRotation = pathPoint.getParentFrame().getRotationInGround(getState());
            const SimTK::InverseRotation& groundToParentRotation = parentToGroundRotation.invert();
            Vec3 const translationInParent = ToVec3(groundToParentRotation * ToSimTKVec3(deltaTranslationInGround));

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

    // manipulator for `OpenSim::PhysicalOffsetFrame`
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

        Mat4 implGetCurrentModelMatrix(
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
            const Vec3& deltaTranslationInGround) final
        {
            SimTK::Rotation parentToGroundRotation = pof.getParentFrame().getRotationInGround(getState());
            if (m_IsChildFrameOfJoint) {
                parentToGroundRotation = NegateRotation(parentToGroundRotation);
            }
            const SimTK::InverseRotation& groundToParentRotation = parentToGroundRotation.invert();
            SimTK::Vec3 const deltaTranslationInParent = groundToParentRotation * ToSimTKVec3(deltaTranslationInGround);
            const SimTK::Vec3& eulersInPofFrame = pof.get_orientation();

            ActionTransformPof(
                getUndoableModel(),
                pof,
                ToVec3(deltaTranslationInParent),
                ToVec3(eulersInPofFrame)
            );
        }

        void implOnApplyRotation(
            const OpenSim::PhysicalOffsetFrame& pof,
            const Eulers& deltaEulerRadiansInGround) final
        {
            const OpenSim::Frame& parent = pof.getParentFrame();
            const SimTK::State& state = getState();

            Quat const deltaRotationInGround = to_worldspace_rotation_quat(m_IsChildFrameOfJoint ? -deltaEulerRadiansInGround : deltaEulerRadiansInGround);
            Quat const oldRotationInGround = ToQuat(pof.getRotationInGround(state));
            Quat const parentRotationInGround = ToQuat(parent.getRotationInGround(state));
            Quat const newRotationInGround = normalize(deltaRotationInGround * oldRotationInGround);
            Quat const newRotationInParent = inverse(parentRotationInGround) * newRotationInGround;

            ActionTransformPof(
                getUndoableModel(),
                pof,
                Vec3{},  // no translation delta
                extract_eulers_xyz(newRotationInParent)
            );
        }

        void implOnSave(const OpenSim::PhysicalOffsetFrame& pof) final
        {
            std::stringstream ss;
            ss << "transformed " << pof.getName();
            getUndoableModel().commit(std::move(ss).str());
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

        Mat4 implGetCurrentModelMatrix(
            const OpenSim::WrapObject& wrapObj) const final
        {
            const SimTK::Transform& wrapToFrame = wrapObj.getTransform();
            SimTK::Transform const frameToGround = wrapObj.getFrame().getTransformInGround(getState());
            SimTK::Transform const wrapToGround = frameToGround * wrapToFrame;

            return ToMat4x4(wrapToGround);
        }

        void implOnApplyTranslation(
            const OpenSim::WrapObject& wrapObj,
            const Vec3& deltaTranslationInGround) final
        {
            SimTK::Rotation const frameToGroundRotation = wrapObj.getFrame().getTransformInGround(getState()).R();
            const SimTK::InverseRotation& groundToFrameRotation = frameToGroundRotation.invert();
            Vec3 const translationInPofFrame = ToVec3(groundToFrameRotation * ToSimTKVec3(deltaTranslationInGround));

            ActionTransformWrapObject(
                getUndoableModel(),
                wrapObj,
                translationInPofFrame,
                ToVec3(wrapObj.get_xyz_body_rotation())
            );
        }

        void implOnApplyRotation(
            const OpenSim::WrapObject& wrapObj,
            const Eulers& deltaEulerRadiansInGround) final
        {
            const OpenSim::Frame& parent = wrapObj.getFrame();
            const SimTK::State& state = getState();

            Quat const deltaRotationInGround = to_worldspace_rotation_quat(deltaEulerRadiansInGround);
            Quat const oldRotationInGround = ToQuat(parent.getTransformInGround(state).R() * wrapObj.getTransform().R());
            Quat const parentRotationInGround = ToQuat(parent.getRotationInGround(state));
            Quat const newRotationInGround = normalize(deltaRotationInGround * oldRotationInGround);
            Quat const newRotationInParent = inverse(parentRotationInGround) * newRotationInGround;

            ActionTransformWrapObject(
                getUndoableModel(),
                wrapObj,
                Vec3{},  // no translation
                extract_eulers_xyz(newRotationInParent)
            );
        }

        void implOnSave(
            const OpenSim::WrapObject& wrapObj) final
        {
            std::stringstream ss;
            ss << "transformed " << wrapObj.getName();
            getUndoableModel().commit(std::move(ss).str());
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

        Mat4 implGetCurrentModelMatrix(
            const OpenSim::ContactGeometry& contactGeom) const final
        {
            SimTK::Transform const wrapToFrame = contactGeom.getTransform();
            SimTK::Transform const frameToGround = contactGeom.getFrame().getTransformInGround(getState());
            SimTK::Transform const wrapToGround = frameToGround * wrapToFrame;

            return ToMat4x4(wrapToGround);
        }

        void implOnApplyTranslation(
            const OpenSim::ContactGeometry& contactGeom,
            const Vec3& deltaTranslationInGround) final
        {
            SimTK::Rotation const frameToGroundRotation = contactGeom.getFrame().getTransformInGround(getState()).R();
            const SimTK::InverseRotation& groundToFrameRotation = frameToGroundRotation.invert();
            Vec3 const translationInPofFrame = ToVec3(groundToFrameRotation * ToSimTKVec3(deltaTranslationInGround));

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

            Quat const deltaRotationInGround = to_worldspace_rotation_quat(deltaEulerRadiansInGround);
            Quat const oldRotationInGround = ToQuat(parent.getTransformInGround(state).R() * contactGeom.getTransform().R());
            Quat const parentRotationInGround = ToQuat(parent.getRotationInGround(state));
            Quat const newRotationInGround = normalize(deltaRotationInGround * oldRotationInGround);
            Quat const newRotationInParent = inverse(parentRotationInGround) * newRotationInGround;

            ActionTransformContactGeometry(
                getUndoableModel(),
                contactGeom,
                Vec3{},  // no translation
                extract_eulers_xyz(newRotationInParent)
            );
        }

        void implOnSave(
            const OpenSim::ContactGeometry& contactGeom) final
        {
            std::stringstream ss;
            ss << "transformed " << contactGeom.getName();
            getUndoableModel().commit(std::move(ss).str());
        }
    };
}

// drawing/rendering code
namespace
{
    void DrawGizmoOverlayInner(
        void* gizmoID,
        const PolarPerspectiveCamera& camera,
        const Rect& viewportRect,
        ImGuizmo::OPERATION operation,
        ImGuizmo::MODE mode,
        ISelectionManipulator& manipulator,
        bool& wasUsingLastFrameStorage)
    {
        // figure out whether the gizmo should even be drawn
        {
            SupportedManipulationOpFlags const flags = manipulator.getSupportedManipulationOps();
            if (operation == ImGuizmo::TRANSLATE && !(flags & SupportedManipulationOpFlags::Translation))
            {
                return;
            }
            if (operation == ImGuizmo::ROTATE && !(flags & SupportedManipulationOpFlags::Rotation))
            {
                return;
            }
        }
        // else: it's a supported operation and the gizmo should be drawn

        // important: necessary for multi-viewport gizmos
        // also important: don't use ui::get_id(), because it uses an ID stack and we might want to know if "isover" etc. is true outside of a window
        ImGuizmo::SetID(static_cast<int>(std::hash<void*>{}(gizmoID)));
        ScopeGuard const g{[]() { ImGuizmo::SetID(-1); }};

        ImGuizmo::SetRect(
            viewportRect.p1.x,
            viewportRect.p1.y,
            dimensions_of(viewportRect).x,
            dimensions_of(viewportRect).y
        );
        ImGuizmo::SetDrawlist(ui::get_panel_draw_list());
        ImGuizmo::AllowAxisFlip(false);

        // use rotation from the parent, translation from station
        Mat4 currentXformInGround = manipulator.getCurrentModelMatrix();
        Mat4 deltaInGround;

        ui::set_gizmo_style_to_osc_standard();
        bool const gizmoWasManipulatedByUser = ImGuizmo::Manipulate(
            value_ptr(camera.view_matrix()),
            value_ptr(camera.projection_matrix(aspect_ratio_of(viewportRect))),
            operation,
            mode,
            value_ptr(currentXformInGround),
            value_ptr(deltaInGround),
            nullptr,
            nullptr,
            nullptr
        );

        bool const isUsingThisFrame = ImGuizmo::IsUsing();
        bool const wasUsingLastFrame = wasUsingLastFrameStorage;
        wasUsingLastFrameStorage = isUsingThisFrame;  // update cached state

        if (wasUsingLastFrame && !isUsingThisFrame)
        {
            // user finished interacting, save model
            manipulator.onSave();
        }

        if (!gizmoWasManipulatedByUser)
        {
            return;  // user is not interacting, so no changes to apply
        }
        // else: apply in-place change to model

        // decompose the overall transformation into component parts
        Vec3 translationInGround{};
        Vec3 rotationInGroundDegrees{};
        Vec3 scaleInGround{};
        ImGuizmo::DecomposeMatrixToComponents(
            value_ptr(deltaInGround),
            value_ptr(translationInGround),
            value_ptr(rotationInGroundDegrees),
            value_ptr(scaleInGround)
        );
        Eulers rotationInGround = Vec<3, Degrees>(rotationInGroundDegrees);

        if (operation == ImGuizmo::TRANSLATE)
        {
            manipulator.onApplyTranslation(translationInGround);
        }
        else if (operation == ImGuizmo::ROTATE)
        {
            manipulator.onApplyRotation(rotationInGround);
        }
    }

    void DrawGizmoOverlay(
        void* gizmoID,
        const PolarPerspectiveCamera& camera,
        const Rect& viewportRect,
        ImGuizmo::OPERATION operation,
        ImGuizmo::MODE mode,
        std::shared_ptr<UndoableModelStatePair> const& model,
        const OpenSim::Component& selected,
        bool& wasUsingLastFrameStorage)
    {
        // use downcasting to figure out which gizmo implementation to use
        if (auto const* const maybeStation = dynamic_cast<OpenSim::Station const*>(&selected))
        {
            StationManipulator manipulator{model, *maybeStation};
            DrawGizmoOverlayInner(gizmoID, camera, viewportRect, operation, mode, manipulator, wasUsingLastFrameStorage);
        }
        else if (auto const* const maybePathPoint = dynamic_cast<OpenSim::PathPoint const*>(&selected))
        {
            PathPointManipulator manipulator{model, *maybePathPoint};
            DrawGizmoOverlayInner(gizmoID, camera, viewportRect, operation, mode, manipulator, wasUsingLastFrameStorage);
        }
        else if (auto const* const maybePof = dynamic_cast<OpenSim::PhysicalOffsetFrame const*>(&selected))
        {
            PhysicalOffsetFrameManipulator manipulator{model, *maybePof};
            DrawGizmoOverlayInner(gizmoID, camera, viewportRect, operation, mode, manipulator, wasUsingLastFrameStorage);
        }
        else if (auto const* const maybeWrapObject = dynamic_cast<OpenSim::WrapObject const*>(&selected))
        {
            WrapObjectManipulator manipulator{model, *maybeWrapObject};
            DrawGizmoOverlayInner(gizmoID, camera, viewportRect, operation, mode, manipulator, wasUsingLastFrameStorage);
        }
        else if (auto const* const maybeContactGeom = dynamic_cast<OpenSim::ContactGeometry const*>(&selected))
        {
            ContactGeometryManipulator manipulator{model, *maybeContactGeom};
            DrawGizmoOverlayInner(gizmoID, camera, viewportRect, operation, mode, manipulator, wasUsingLastFrameStorage);
        }
        else
        {
            // (do nothing: we don't know how to manipulate the selection0
        }
    }
}

osc::ModelSelectionGizmo::ModelSelectionGizmo(std::shared_ptr<UndoableModelStatePair> model_) :
    m_Model{std::move(model_)}
{
}

osc::ModelSelectionGizmo::ModelSelectionGizmo(const ModelSelectionGizmo&) = default;
osc::ModelSelectionGizmo::ModelSelectionGizmo(ModelSelectionGizmo&&) noexcept = default;
osc::ModelSelectionGizmo& osc::ModelSelectionGizmo::operator=(const ModelSelectionGizmo&) = default;
osc::ModelSelectionGizmo& osc::ModelSelectionGizmo::operator=(ModelSelectionGizmo&&) noexcept = default;
osc::ModelSelectionGizmo::~ModelSelectionGizmo() noexcept = default;

bool osc::ModelSelectionGizmo::isUsing() const
{
    ImGuizmo::SetID(static_cast<int>(std::hash<ModelSelectionGizmo const*>{}(this)));
    bool const rv = ImGuizmo::IsUsing();
    ImGuizmo::SetID(-1);
    return rv;
}

bool osc::ModelSelectionGizmo::isOver() const
{
    ImGuizmo::SetID(static_cast<int>(std::hash<ModelSelectionGizmo const*>{}(this)));
    bool const rv = ImGuizmo::IsOver();
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
    OpenSim::Component const* selected = m_Model->getSelected();
    if (!selected)
    {
        return;
    }

    DrawGizmoOverlay(
        this,
        camera,
        screenRect,
        m_GizmoOperation,
        m_GizmoMode,
        m_Model,
        *selected,
        m_WasUsingGizmoLastFrame
    );
}

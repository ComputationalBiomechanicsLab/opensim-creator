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
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/PolarPerspectiveCamera.h>
#include <oscar/Maths/Quat.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Vec.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>
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
        ISelectionManipulator(ISelectionManipulator const&) = default;
        ISelectionManipulator(ISelectionManipulator&&) noexcept = default;
        ISelectionManipulator& operator=(ISelectionManipulator const&) = default;
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

        void onApplyTranslation(Vec3 const& deltaTranslationInGround)
        {
            implOnApplyTranslation(deltaTranslationInGround);
        }

        void onApplyRotation(Eulers const& deltaEulerRadiansInGround)
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
        virtual void implOnApplyTranslation(Vec3 const&) {}  // default to noop
        virtual void implOnApplyRotation(Eulers const&) {}  // default to noop
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
            TComponent const& component) :

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

        OpenSim::Model const& getModel() const
        {
            return m_Model->getModel();
        }

        SimTK::State const& getState() const
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
                return Identity<Mat4>();  // selection of that type does not exist in the model
            }
            return implGetCurrentModelMatrix(*maybeSelected);
        }

        // perform runtime lookup for `TComponent` and forward into concrete implementation
        void implOnApplyTranslation(Vec3 const& deltaTranslationInGround) final
        {
            TComponent const* maybeSelected = findSelection();
            if (!maybeSelected)
            {
                return;  // selection of that type does not exist in the model
            }
            implOnApplyTranslation(*maybeSelected, deltaTranslationInGround);
        }

        // perform runtime lookup for `TComponent` and forward into concrete implementation
        void implOnApplyRotation(Eulers const& deltaEulerRadiansInGround) final
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
        virtual Mat4 implGetCurrentModelMatrix(TComponent const&) const = 0;
        virtual void implOnApplyTranslation(TComponent const&, Vec3 const& deltaTranslationInGround) = 0;
        virtual void implOnApplyRotation(TComponent const&, Eulers const&) {}  // default to noop
        virtual void implOnSave(TComponent const&) = 0;

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
            OpenSim::Station const& station) :
            SelectionManipulator{std::move(model_), station}
        {}

    private:

        SupportedManipulationOpFlags implGetSupportedManipulationOps() const final
        {
            return SupportedManipulationOpFlags::Translation;
        }

        Mat4 implGetCurrentModelMatrix(
            OpenSim::Station const& station) const final
        {
            SimTK::State const& state = getState();
            Mat4 currentXformInGround = ToMat4(station.getParentFrame().getRotationInGround(state));
            currentXformInGround[3] = Vec4{ToVec3(station.getLocationInGround(state)), 1.0f};

            return currentXformInGround;
        }

        void implOnApplyTranslation(
            OpenSim::Station const& station,
            Vec3 const& deltaTranslationInGround) final
        {
            SimTK::Rotation const parentToGroundRotation = station.getParentFrame().getRotationInGround(getState());
            SimTK::InverseRotation const& groundToParentRotation = parentToGroundRotation.invert();
            Vec3 const translationInParent = ToVec3(groundToParentRotation * ToSimTKVec3(deltaTranslationInGround));

            ActionTranslateStation(getUndoableModel(), station, translationInParent);
        }

        void implOnSave(OpenSim::Station const& station) final
        {
            ActionTranslateStationAndSave(getUndoableModel(), station, {});
        }
    };

    // manipulator for `OpenSim::PathPoint`
    class PathPointManipulator : public SelectionManipulator<OpenSim::PathPoint> {
    public:
        PathPointManipulator(
            std::shared_ptr<UndoableModelStatePair> model_,
            OpenSim::PathPoint const& pathPoint) :
            SelectionManipulator{std::move(model_), pathPoint}
        {}

    private:
        SupportedManipulationOpFlags implGetSupportedManipulationOps() const final
        {
            return SupportedManipulationOpFlags::Translation;
        }

        Mat4 implGetCurrentModelMatrix(
            OpenSim::PathPoint const& pathPoint) const final
        {
            SimTK::State const& state = getState();
            Mat4 currentXformInGround = ToMat4(pathPoint.getParentFrame().getRotationInGround(state));
            currentXformInGround[3] = Vec4{ToVec3(pathPoint.getLocationInGround(state)), 1.0f};

            return currentXformInGround;
        }

        void implOnApplyTranslation(
            OpenSim::PathPoint const& pathPoint,
            Vec3 const& deltaTranslationInGround) final
        {
            SimTK::Rotation const parentToGroundRotation = pathPoint.getParentFrame().getRotationInGround(getState());
            SimTK::InverseRotation const& groundToParentRotation = parentToGroundRotation.invert();
            Vec3 const translationInParent = ToVec3(groundToParentRotation * ToSimTKVec3(deltaTranslationInGround));

            ActionTranslatePathPoint(getUndoableModel(), pathPoint, translationInParent);
        }

        void implOnSave(OpenSim::PathPoint const& pathPoint) final
        {
            ActionTranslatePathPointAndSave(getUndoableModel(), pathPoint, {});
        }
    };

    bool IsDirectChildOfAnyJoint(OpenSim::Model const& model, OpenSim::Frame const& frame)
    {
        for (OpenSim::Joint const& joint : model.getComponentList<OpenSim::Joint>()) {
            if (&joint.getChildFrame() == &frame) {
                return true;
            }
        }
        return false;
    }

    SimTK::Rotation NegateRotation(SimTK::Rotation const& r)
    {
        return SimTK::Rotation{-SimTK::Mat33{r}, true};
    }

    // manipulator for `OpenSim::PhysicalOffsetFrame`
    class PhysicalOffsetFrameManipulator final : public SelectionManipulator<OpenSim::PhysicalOffsetFrame> {
    public:
        PhysicalOffsetFrameManipulator(
            std::shared_ptr<UndoableModelStatePair> model_,
            OpenSim::PhysicalOffsetFrame const& pof) :
            SelectionManipulator{std::move(model_), pof},
            m_IsChildFrameOfJoint{IsDirectChildOfAnyJoint(getModel(), pof)}
        {}

    private:
        SupportedManipulationOpFlags implGetSupportedManipulationOps() const final
        {
            return SupportedManipulationOpFlags::Translation | SupportedManipulationOpFlags::Rotation;
        }

        Mat4 implGetCurrentModelMatrix(
            OpenSim::PhysicalOffsetFrame const& pof) const final
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
            OpenSim::PhysicalOffsetFrame const& pof,
            Vec3 const& deltaTranslationInGround) final
        {
            SimTK::Rotation parentToGroundRotation = pof.getParentFrame().getRotationInGround(getState());
            if (m_IsChildFrameOfJoint) {
                parentToGroundRotation = NegateRotation(parentToGroundRotation);
            }
            SimTK::InverseRotation const& groundToParentRotation = parentToGroundRotation.invert();
            SimTK::Vec3 const deltaTranslationInParent = groundToParentRotation * ToSimTKVec3(deltaTranslationInGround);
            SimTK::Vec3 const& eulersInPofFrame = pof.get_orientation();

            ActionTransformPof(
                getUndoableModel(),
                pof,
                ToVec3(deltaTranslationInParent),
                ToVec3(eulersInPofFrame)
            );
        }

        void implOnApplyRotation(
            OpenSim::PhysicalOffsetFrame const& pof,
            Eulers const& deltaEulerRadiansInGround) final
        {
            OpenSim::Frame const& parent = pof.getParentFrame();
            SimTK::State const& state = getState();

            Quat const deltaRotationInGround = WorldspaceRotation(m_IsChildFrameOfJoint ? -deltaEulerRadiansInGround : deltaEulerRadiansInGround);
            Quat const oldRotationInGround = ToQuat(pof.getRotationInGround(state));
            Quat const parentRotationInGround = ToQuat(parent.getRotationInGround(state));
            Quat const newRotationInGround = normalize(deltaRotationInGround * oldRotationInGround);
            Quat const newRotationInParent = Inverse(parentRotationInGround) * newRotationInGround;

            ActionTransformPof(
                getUndoableModel(),
                pof,
                Vec3{},  // no translation delta
                ExtractEulerAngleXYZ(newRotationInParent)
            );
        }

        void implOnSave(OpenSim::PhysicalOffsetFrame const& pof) final
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
            OpenSim::WrapObject const& wo) :
            SelectionManipulator{std::move(model_), wo}
        {}

    private:
        SupportedManipulationOpFlags implGetSupportedManipulationOps() const final
        {
            return SupportedManipulationOpFlags::Rotation | SupportedManipulationOpFlags::Translation;
        }

        Mat4 implGetCurrentModelMatrix(
            OpenSim::WrapObject const& wrapObj) const final
        {
            SimTK::Transform const& wrapToFrame = wrapObj.getTransform();
            SimTK::Transform const frameToGround = wrapObj.getFrame().getTransformInGround(getState());
            SimTK::Transform const wrapToGround = frameToGround * wrapToFrame;

            return ToMat4x4(wrapToGround);
        }

        void implOnApplyTranslation(
            OpenSim::WrapObject const& wrapObj,
            Vec3 const& deltaTranslationInGround) final
        {
            SimTK::Rotation const frameToGroundRotation = wrapObj.getFrame().getTransformInGround(getState()).R();
            SimTK::InverseRotation const& groundToFrameRotation = frameToGroundRotation.invert();
            Vec3 const translationInPofFrame = ToVec3(groundToFrameRotation * ToSimTKVec3(deltaTranslationInGround));

            ActionTransformWrapObject(
                getUndoableModel(),
                wrapObj,
                translationInPofFrame,
                ToVec3(wrapObj.get_xyz_body_rotation())
            );
        }

        void implOnApplyRotation(
            OpenSim::WrapObject const& wrapObj,
            Eulers const& deltaEulerRadiansInGround) final
        {
            OpenSim::Frame const& parent = wrapObj.getFrame();
            SimTK::State const& state = getState();

            Quat const deltaRotationInGround = WorldspaceRotation(deltaEulerRadiansInGround);
            Quat const oldRotationInGround = ToQuat(parent.getTransformInGround(state).R() * wrapObj.getTransform().R());
            Quat const parentRotationInGround = ToQuat(parent.getRotationInGround(state));
            Quat const newRotationInGround = normalize(deltaRotationInGround * oldRotationInGround);
            Quat const newRotationInParent = Inverse(parentRotationInGround) * newRotationInGround;

            ActionTransformWrapObject(
                getUndoableModel(),
                wrapObj,
                Vec3{},  // no translation
                ExtractEulerAngleXYZ(newRotationInParent)
            );
        }

        void implOnSave(
            OpenSim::WrapObject const& wrapObj) final
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
            OpenSim::ContactGeometry const& contactGeom) :
            SelectionManipulator{std::move(model_), contactGeom}
        {}

    private:
        SupportedManipulationOpFlags implGetSupportedManipulationOps() const final
        {
            return SupportedManipulationOpFlags::Rotation | SupportedManipulationOpFlags::Translation;
        }

        Mat4 implGetCurrentModelMatrix(
            OpenSim::ContactGeometry const& contactGeom) const final
        {
            SimTK::Transform const wrapToFrame = contactGeom.getTransform();
            SimTK::Transform const frameToGround = contactGeom.getFrame().getTransformInGround(getState());
            SimTK::Transform const wrapToGround = frameToGround * wrapToFrame;

            return ToMat4x4(wrapToGround);
        }

        void implOnApplyTranslation(
            OpenSim::ContactGeometry const& contactGeom,
            Vec3 const& deltaTranslationInGround) final
        {
            SimTK::Rotation const frameToGroundRotation = contactGeom.getFrame().getTransformInGround(getState()).R();
            SimTK::InverseRotation const& groundToFrameRotation = frameToGroundRotation.invert();
            Vec3 const translationInPofFrame = ToVec3(groundToFrameRotation * ToSimTKVec3(deltaTranslationInGround));

            ActionTransformContactGeometry(
                getUndoableModel(),
                contactGeom,
                translationInPofFrame,
                ToVec3(contactGeom.get_orientation())
            );
        }

        void implOnApplyRotation(
            OpenSim::ContactGeometry const& contactGeom,
            Eulers const& deltaEulerRadiansInGround) final
        {
            OpenSim::Frame const& parent = contactGeom.getFrame();
            SimTK::State const& state = getState();

            Quat const deltaRotationInGround = WorldspaceRotation(deltaEulerRadiansInGround);
            Quat const oldRotationInGround = ToQuat(parent.getTransformInGround(state).R() * contactGeom.getTransform().R());
            Quat const parentRotationInGround = ToQuat(parent.getRotationInGround(state));
            Quat const newRotationInGround = normalize(deltaRotationInGround * oldRotationInGround);
            Quat const newRotationInParent = Inverse(parentRotationInGround) * newRotationInGround;

            ActionTransformContactGeometry(
                getUndoableModel(),
                contactGeom,
                Vec3{},  // no translation
                ExtractEulerAngleXYZ(newRotationInParent)
            );
        }

        void implOnSave(
            OpenSim::ContactGeometry const& contactGeom) final
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
        PolarPerspectiveCamera const& camera,
        Rect const& viewportRect,
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
        // also important: don't use ImGui::GetID(), because it uses an ID stack and we might want to know if "isover" etc. is true outside of a window
        ImGuizmo::SetID(static_cast<int>(std::hash<void*>{}(gizmoID)));
        ScopeGuard const g{[]() { ImGuizmo::SetID(-1); }};

        ImGuizmo::SetRect(
            viewportRect.p1.x,
            viewportRect.p1.y,
            Dimensions(viewportRect).x,
            Dimensions(viewportRect).y
        );
        ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
        ImGuizmo::AllowAxisFlip(false);

        // use rotation from the parent, translation from station
        Mat4 currentXformInGround = manipulator.getCurrentModelMatrix();
        Mat4 deltaInGround;

        SetImguizmoStyleToOSCStandard();
        bool const gizmoWasManipulatedByUser = ImGuizmo::Manipulate(
            ValuePtr(camera.getViewMtx()),
            ValuePtr(camera.getProjMtx(AspectRatio(viewportRect))),
            operation,
            mode,
            ValuePtr(currentXformInGround),
            ValuePtr(deltaInGround),
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
            ValuePtr(deltaInGround),
            ValuePtr(translationInGround),
            ValuePtr(rotationInGroundDegrees),
            ValuePtr(scaleInGround)
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
        PolarPerspectiveCamera const& camera,
        Rect const& viewportRect,
        ImGuizmo::OPERATION operation,
        ImGuizmo::MODE mode,
        std::shared_ptr<UndoableModelStatePair> const& model,
        OpenSim::Component const& selected,
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

osc::ModelSelectionGizmo::ModelSelectionGizmo(ModelSelectionGizmo const&) = default;
osc::ModelSelectionGizmo::ModelSelectionGizmo(ModelSelectionGizmo&&) noexcept = default;
osc::ModelSelectionGizmo& osc::ModelSelectionGizmo::operator=(ModelSelectionGizmo const&) = default;
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
    return osc::UpdateImguizmoStateFromKeyboard(m_GizmoOperation, m_GizmoMode);
}

void osc::ModelSelectionGizmo::onDraw(
    Rect const& screenRect,
    PolarPerspectiveCamera const& camera)
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

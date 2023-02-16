#include "ModelSelectionGizmo.hpp"

#include "src/Bindings/ImGuizmoHelpers.hpp"
#include "src/Maths/MathHelpers.hpp"
#include "src/Maths/PolarPerspectiveCamera.hpp"
#include "src/Maths/Rect.hpp"
#include "src/OpenSimBindings/ActionFunctions.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/SimTKHelpers.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Utils/Assertions.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <imgui.h>
#include <ImGuizmo.h>  // care: must be included after imgui
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/Marker.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PathPoint.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <OpenSim/Simulation/Model/ContactGeometry.h>
#include <OpenSim/Simulation/Wrap/WrapObject.h>
#include <Simbody.h>

#include <cstddef>
#include <memory>
#include <sstream>
#include <utility>

namespace
{
    // operations that are supported by a manipulator
    using SupportedManipulationOpFlags = int32_t;
    enum SupportedManipulationOpFlags_ : int32_t {
        SupportedManipulationOpFlags_None = 0,
        SupportedManipulationOpFlags_Translation = 1<<0,
        SupportedManipulationOpFlags_Rotation = 1<<1,
    };

    // virtual base class that each concrete manipulator inherits from
    class VirtualSelectionManipulator {
    protected:
        VirtualSelectionManipulator() = default;
        VirtualSelectionManipulator(VirtualSelectionManipulator const&) = default;
        VirtualSelectionManipulator(VirtualSelectionManipulator&&) noexcept = default;
        VirtualSelectionManipulator& operator=(VirtualSelectionManipulator const&) = default;
        VirtualSelectionManipulator& operator=(VirtualSelectionManipulator&&) noexcept = default;
    public:
        virtual ~VirtualSelectionManipulator() noexcept = default;

        SupportedManipulationOpFlags getSupportedManipulationOps() const
        {
            return implGetSupportedManipulationOps();
        }

        glm::mat4 getCurrentModelMatrix() const
        {
            return implGetCurrentModelMatrix();
        }

        void onApplyTranslation(glm::vec3 const& deltaTranslationInGround)
        {
            implOnApplyTranslation(deltaTranslationInGround);
        }

        void onApplyRotation(glm::vec3 const& newEulerRadiansInGround)
        {
            implOnApplyRotation(newEulerRadiansInGround);
        }

        void onSave()
        {
            implOnSave();
        }
    private:
        virtual SupportedManipulationOpFlags implGetSupportedManipulationOps() const = 0;
        virtual glm::mat4 implGetCurrentModelMatrix() const = 0;
        virtual void implOnApplyTranslation(glm::vec3 const& deltaTranslationInGround) {}  // default to noop
        virtual void implOnApplyRotation(glm::vec3 const& newEulerRadiansInGround) {}  // default to noop
        virtual void implOnSave() = 0;
    };

    // standard implementation of the data storage for a selection manipulator
    //
    // effectively, only stores the model+path to the thing being manipulated, and performs
    // runtime checks to ensure the component still exists in the model
    template<typename TComponent>
    class StandardSelectionManipulatorImpl : public VirtualSelectionManipulator {
    protected:
        StandardSelectionManipulatorImpl(
            std::shared_ptr<osc::UndoableModelStatePair> model_,
            TComponent const& component) :

            m_Model{std::move(model_)},
            m_ComponentAbsPath{component.getAbsolutePath()}
        {
            OSC_ASSERT(osc::FindComponent<TComponent>(m_Model->getModel(), m_ComponentAbsPath));
        }

        TComponent const* findSelection() const
        {
            return osc::FindComponent<TComponent>(m_Model->getModel(), m_ComponentAbsPath);
        }

        OpenSim::Model const& getModel() const
        {
            return m_Model->getModel();
        }

        SimTK::State const& getState() const
        {
            return m_Model->getState();
        }

        osc::UndoableModelStatePair& getUndoableModel()
        {
            return *m_Model;
        }

    private:
        std::shared_ptr<osc::UndoableModelStatePair> m_Model;
        OpenSim::ComponentPath m_ComponentAbsPath;
    };

    class StationManipulator final : public StandardSelectionManipulatorImpl<OpenSim::Station> {
    public:
        StationManipulator(
            std::shared_ptr<osc::UndoableModelStatePair> model_,
            OpenSim::Station const& station) :
            StandardSelectionManipulatorImpl{std::move(model_), station}
        {
        }
    private:

        SupportedManipulationOpFlags implGetSupportedManipulationOps() const final
        {
            return SupportedManipulationOpFlags_Translation;
        }

        glm::mat4 implGetCurrentModelMatrix() const final
        {
            OpenSim::Station const* maybeStation = findSelection();
            if (!maybeStation)
            {
                return glm::mat4{1.0f};
            }
            OpenSim::Station const& station = *maybeStation;

            SimTK::State const& state = getState();
            glm::mat4 currentXformInGround = osc::ToMat4(station.getParentFrame().getRotationInGround(state));
            currentXformInGround[3] = glm::vec4{osc::ToVec3(station.getLocationInGround(state)), 1.0f};

            return currentXformInGround;
        }

        void implOnApplyTranslation(glm::vec3 const& deltaTranslationInGround) final
        {
            OpenSim::Station const* maybeStation = findSelection();
            if (!maybeStation)
            {
                return;  // station doesn't exist in the model?
            }
            OpenSim::Station const& station = *maybeStation;

            SimTK::Rotation const parentToGroundRotation = station.getParentFrame().getRotationInGround(getState());
            SimTK::InverseRotation const& groundToParentRotation = parentToGroundRotation.invert();
            glm::vec3 const translationInParent = osc::ToVec3(groundToParentRotation * osc::ToSimTKVec3(deltaTranslationInGround));

            ActionTranslateStation(getUndoableModel(), station, translationInParent);
        }

        void implOnSave() final
        {
            if (OpenSim::Station const* station = findSelection())
            {
                ActionTranslateStationAndSave(getUndoableModel(), *station, {});
            }
        }
    };

    class PathPointManipulator : public StandardSelectionManipulatorImpl<OpenSim::PathPoint> {
    public:
        PathPointManipulator(
            std::shared_ptr<osc::UndoableModelStatePair> model_,
            OpenSim::PathPoint const& pathPoint) :
            StandardSelectionManipulatorImpl{std::move(model_), pathPoint}
        {
        }
    private:
        SupportedManipulationOpFlags implGetSupportedManipulationOps() const final
        {
            return SupportedManipulationOpFlags_Translation;
        }

        glm::mat4 implGetCurrentModelMatrix() const final
        {
            OpenSim::PathPoint const* maybePathPoint = findSelection();
            if (!maybePathPoint)
            {
                return glm::mat4{1.0f};  // pathpoint is no longer in the model?
            }
            OpenSim::PathPoint const& pathPoint = *maybePathPoint;

            SimTK::State const& state = getState();
            glm::mat4 currentXformInGround = osc::ToMat4(pathPoint.getParentFrame().getRotationInGround(state));
            currentXformInGround[3] = glm::vec4{osc::ToVec3(pathPoint.getLocationInGround(state)), 1.0f};

            return currentXformInGround;
        }

        void implOnApplyTranslation(glm::vec3 const& deltaTranslationInGround) final
        {
            OpenSim::PathPoint const* maybePathPoint = findSelection();
            if (!maybePathPoint)
            {
                return;  // pathpoint is no longer in the model?
            }
            OpenSim::PathPoint const& pathPoint = *maybePathPoint;

            SimTK::Rotation const parentToGroundRotation = pathPoint.getParentFrame().getRotationInGround(getState());
            SimTK::InverseRotation const& groundToParentRotation = parentToGroundRotation.invert();
            glm::vec3 const translationInParent = osc::ToVec3(groundToParentRotation * osc::ToSimTKVec3(deltaTranslationInGround));
            ActionTranslatePathPoint(getUndoableModel(), pathPoint, translationInParent);
        }

        void implOnSave() final
        {
            OpenSim::PathPoint const* maybePathPoint = findSelection();
            if (!maybePathPoint)
            {
                return;  // pathpoint is no longer in the model?
            }
            OpenSim::PathPoint const& pathPoint = *maybePathPoint;

            ActionTranslatePathPointAndSave(getUndoableModel(), pathPoint, {});
        }
    };

    class PhysicalOffsetFrameManipulator final : public StandardSelectionManipulatorImpl<OpenSim::PhysicalOffsetFrame> {
    public:
        PhysicalOffsetFrameManipulator(
            std::shared_ptr<osc::UndoableModelStatePair> model_,
            OpenSim::PhysicalOffsetFrame const& pof) :
            StandardSelectionManipulatorImpl{std::move(model_), pof}
        {
        }
    private:
        SupportedManipulationOpFlags implGetSupportedManipulationOps() const final
        {
            return SupportedManipulationOpFlags_Translation | SupportedManipulationOpFlags_Rotation;
        }

        glm::mat4 implGetCurrentModelMatrix() const final
        {
            OpenSim::PhysicalOffsetFrame const* maybePof = findSelection();
            if (!maybePof)
            {
                return glm::mat4{1.0f};  // pof doesn't exist in the model
            }
            OpenSim::PhysicalOffsetFrame const& pof = *maybePof;

            // use the PoF's own rotation as its local space
            SimTK::State const& state = getState();
            glm::mat4 currentXformInGround = osc::ToMat4(pof.getRotationInGround(state));
            currentXformInGround[3] = glm::vec4{osc::ToVec3(pof.getPositionInGround(state)), 1.0f};
            return currentXformInGround;
        }

        void implOnApplyTranslation(glm::vec3 const& deltaTranslationInGround) final
        {
            OpenSim::PhysicalOffsetFrame const* maybePof = findSelection();
            if (!maybePof)
            {
                return;  // pof doesn't exist in the model
            }
            OpenSim::PhysicalOffsetFrame const& pof = *maybePof;

            glm::vec3 translationInPofFrame{};
            glm::vec3 eulersInPofFrame{};
            SimTK::Rotation const pofToGroundRotation =  pof.getRotationInGround(getState());
            SimTK::InverseRotation const& groundToParentRotation = pofToGroundRotation.invert();
            translationInPofFrame = osc::ToVec3(groundToParentRotation * osc::ToSimTKVec3(deltaTranslationInGround));
            eulersInPofFrame = osc::ToVec3(pof.get_orientation());
            ActionTransformPof(getUndoableModel(), pof, translationInPofFrame, eulersInPofFrame);
        }

        void implOnApplyRotation(glm::vec3 const& newEulerRadiansInGround) final
        {
            OpenSim::PhysicalOffsetFrame const* maybePof = findSelection();
            if (!maybePof)
            {
                return;  // pof doesn't exist in the model
            }
            OpenSim::PhysicalOffsetFrame const& pof = *maybePof;

            glm::vec3 translationInPofFrame{};
            glm::vec3 eulersInPofFrame{};
            osc::Transform t = osc::ToTransform(pof.getTransformInGround(getState()));
            ApplyWorldspaceRotation(t, newEulerRadiansInGround, getCurrentModelMatrix()[3]);  // TODO: refactor this garbage
            translationInPofFrame = {};
            eulersInPofFrame = ExtractEulerAngleXYZ(t);
            ActionTransformPof(getUndoableModel(), pof, translationInPofFrame, eulersInPofFrame);
        }

        void implOnSave() final
        {
            OpenSim::PhysicalOffsetFrame const* maybePof = findSelection();
            if (!maybePof)
            {
                return;  // pof doesn't exist in the model
            }
            OpenSim::PhysicalOffsetFrame const& pof = *maybePof;

            std::stringstream ss;
            ss << "transformed " << pof.getName();
            getUndoableModel().commit(std::move(ss).str());
        }
    };

    class WrapObjectManipulator final : public StandardSelectionManipulatorImpl<OpenSim::WrapObject> {
    public:
        WrapObjectManipulator(
            std::shared_ptr<osc::UndoableModelStatePair> model_,
            OpenSim::WrapObject const& wo) :
            StandardSelectionManipulatorImpl{std::move(model_), wo}
        {
        }
    private:
        SupportedManipulationOpFlags implGetSupportedManipulationOps() const final
        {
            return SupportedManipulationOpFlags_Rotation | SupportedManipulationOpFlags_Translation;
        }

        glm::mat4 implGetCurrentModelMatrix() const final
        {
            OpenSim::WrapObject const* maybeWrapObj = findSelection();
            if (!maybeWrapObj)
            {
                return glm::mat4{1.0f};  // wrap object doesn't exist in the model
            }
            OpenSim::WrapObject const& wrapObj = *maybeWrapObj;

            SimTK::Transform const wrapToFrame = wrapObj.getTransform();
            SimTK::Transform const frameToGround = wrapObj.getFrame().getTransformInGround(getState());
            SimTK::Transform const wrapToGround = frameToGround * wrapToFrame;
            return osc::ToMat4x4(wrapToGround);
        }

        void implOnApplyTranslation(glm::vec3 const& deltaTranslationInGround) final
        {
            OpenSim::WrapObject const* maybeWrapObj = findSelection();
            if (!maybeWrapObj)
            {
                return;  // wrap object doesn't exist in the model
            }
            OpenSim::WrapObject const& wrapObj = *maybeWrapObj;

            glm::vec3 translationInPofFrame{};
            glm::vec3 eulersInPofFrame{};
            SimTK::Rotation const frameToGroundRotation = wrapObj.getFrame().getTransformInGround(getState()).R();
            SimTK::InverseRotation const& groundToFrameRotation = frameToGroundRotation.invert();
            translationInPofFrame = osc::ToVec3(groundToFrameRotation * osc::ToSimTKVec3(deltaTranslationInGround));
            eulersInPofFrame = osc::ToVec3(wrapObj.get_xyz_body_rotation());
            ActionTransformWrapObject(getUndoableModel(), wrapObj, translationInPofFrame, eulersInPofFrame);
        }

        void implOnApplyRotation(glm::vec3 const& newEulerRadiansInGround) final
        {
            // TODO: fix this gigantic mess

            OpenSim::WrapObject const* maybeWrapObj = findSelection();
            if (!maybeWrapObj)
            {
                return;  // wrap object doesn't exist in the model
            }
            OpenSim::WrapObject const& wrapObj = *maybeWrapObj;
            SimTK::Transform const wrapToFrame = wrapObj.getTransform();
            SimTK::Transform const frameToGround = wrapObj.getFrame().getTransformInGround(getState());
            SimTK::Transform const wrapToGround = frameToGround * wrapToFrame;

            glm::vec3 translationInPofFrame{};
            glm::vec3 eulersInPofFrame{};
            osc::Transform wrapToGroundXform = osc::ToTransform(wrapToGround);
            ApplyWorldspaceRotation(wrapToGroundXform, newEulerRadiansInGround, osc::ToMat4x4(wrapToGround)[3]);
            SimTK::Transform newWrapToFrameXForm = frameToGround.invert() * osc::ToSimTKTransform(wrapToGroundXform);
            translationInPofFrame = {};
            eulersInPofFrame = osc::ExtractEulerAngleXYZ(osc::ToTransform(newWrapToFrameXForm));
            ActionTransformWrapObject(getUndoableModel(), wrapObj, translationInPofFrame, eulersInPofFrame);
        }

        void implOnSave() final
        {
            OpenSim::WrapObject const* maybeWrapObj = findSelection();
            if (!maybeWrapObj)
            {
                return;  // wrap object doesn't exist in the model
            }
            OpenSim::WrapObject const& wrapObj = *maybeWrapObj;

            std::stringstream ss;
            ss << "transformed " << wrapObj.getName();
            getUndoableModel().commit(std::move(ss).str());
        }
    };

    class ContactGeometryManipulator final : public StandardSelectionManipulatorImpl<OpenSim::ContactGeometry> {
    public:
        ContactGeometryManipulator(
            std::shared_ptr<osc::UndoableModelStatePair> model_,
            OpenSim::ContactGeometry const& contactGeom) :
            StandardSelectionManipulatorImpl{std::move(model_), contactGeom}
        {
        }
    private:
        SupportedManipulationOpFlags implGetSupportedManipulationOps() const final
        {
            return SupportedManipulationOpFlags_Rotation | SupportedManipulationOpFlags_Translation;
        }

        glm::mat4 implGetCurrentModelMatrix() const final
        {
            OpenSim::ContactGeometry const* maybeContactGeom = findSelection();
            if (!maybeContactGeom)
            {
                return glm::mat4{1.0f};  // wrap object doesn't exist in the model
            }
            OpenSim::ContactGeometry const& contactGeom = *maybeContactGeom;

            SimTK::Transform const wrapToFrame = contactGeom.getTransform();
            SimTK::Transform const frameToGround = contactGeom.getFrame().getTransformInGround(getState());
            SimTK::Transform const wrapToGround = frameToGround * wrapToFrame;

            return osc::ToMat4x4(wrapToGround);
        }

        void implOnApplyTranslation(glm::vec3 const& deltaTranslationInGround) final
        {
            OpenSim::ContactGeometry const* maybeContactGeom = findSelection();
            if (!maybeContactGeom)
            {
                return;  // wrap object doesn't exist in the model
            }
            OpenSim::ContactGeometry const& contactGeom = *maybeContactGeom;

            glm::vec3 translationInPofFrame{};
            glm::vec3 eulersInPofFrame{};
            SimTK::Rotation const frameToGroundRotation = contactGeom.getFrame().getTransformInGround(getState()).R();
            SimTK::InverseRotation const& groundToFrameRotation = frameToGroundRotation.invert();
            translationInPofFrame = osc::ToVec3(groundToFrameRotation * osc::ToSimTKVec3(deltaTranslationInGround));
            eulersInPofFrame = osc::ToVec3(contactGeom.get_orientation());

            osc::ActionTransformContactGeometry(
                getUndoableModel(),
                contactGeom,
                translationInPofFrame,
                eulersInPofFrame
            );
        }

        void implOnApplyRotation(glm::vec3 const& newEulerRadiansInGround) final
        {
            // TODO: fix this gigantic mess

            OpenSim::ContactGeometry const* maybeContactGeom = findSelection();
            if (!maybeContactGeom)
            {
                return;  // wrap object doesn't exist in the model
            }
            OpenSim::ContactGeometry const& contactGeom = *maybeContactGeom;
            SimTK::Transform const wrapToFrame = contactGeom.getTransform();
            SimTK::Transform const frameToGround = contactGeom.getFrame().getTransformInGround(getState());
            SimTK::Transform const wrapToGround = frameToGround * wrapToFrame;

            glm::vec3 translationInPofFrame{};
            glm::vec3 eulersInPofFrame{};
            osc::Transform wrapToGroundXform = osc::ToTransform(wrapToGround);
            ApplyWorldspaceRotation(wrapToGroundXform, newEulerRadiansInGround, osc::ToMat4x4(wrapToGround)[3]);
            SimTK::Transform newWrapToFrameXForm = frameToGround.invert() * osc::ToSimTKTransform(wrapToGroundXform);
            translationInPofFrame = {};
            eulersInPofFrame = osc::ExtractEulerAngleXYZ(osc::ToTransform(newWrapToFrameXForm));

            osc::ActionTransformContactGeometry(
                getUndoableModel(),
                contactGeom,
                translationInPofFrame,
                eulersInPofFrame
            );
        }

        void implOnSave() final
        {
            OpenSim::ContactGeometry const* maybeContactGeom = findSelection();
            if (!maybeContactGeom)
            {
                return;  // wrap object doesn't exist in the model
            }
            OpenSim::ContactGeometry const& contactGeom = *maybeContactGeom;

            std::stringstream ss;
            ss << "transformed " << contactGeom.getName();
            getUndoableModel().commit(std::move(ss).str());
        }
    };

    void DrawGizmoOverlayInner(
        void* gizmoID,
        osc::PolarPerspectiveCamera const& camera,
        osc::Rect const& viewportRect,
        ImGuizmo::OPERATION operation,
        ImGuizmo::MODE mode,
        VirtualSelectionManipulator& manipulator,
        bool& wasUsingLastFrameStorage)
    {
        // figure out whether the gizmo should even be drawn
        {
            SupportedManipulationOpFlags const flags = manipulator.getSupportedManipulationOps();
            if (operation == ImGuizmo::TRANSLATE && !(flags & SupportedManipulationOpFlags_Translation))
            {
                return;
            }
            if (operation == ImGuizmo::ROTATE && !(flags & SupportedManipulationOpFlags_Rotation))
            {
                return;
            }
        }
        // else: it's a supported operation and the gizmo should be drawn

        ImGuizmo::SetID(ImGui::GetID(gizmoID));  // important: necessary for multi-viewport gizmos
        ImGuizmo::SetRect(
            viewportRect.p1.x,
            viewportRect.p1.y,
            Dimensions(viewportRect).x,
            Dimensions(viewportRect).y
        );
        ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
        ImGuizmo::AllowAxisFlip(false);

        // use rotation from the parent, translation from station
        glm::mat4 currentXformInGround = manipulator.getCurrentModelMatrix();
        glm::mat4 deltaInGround;

        bool const gizmoWasManipulatedByUser = ImGuizmo::Manipulate(
            glm::value_ptr(camera.getViewMtx()),
            glm::value_ptr(camera.getProjMtx(AspectRatio(viewportRect))),
            operation,
            mode,
            glm::value_ptr(currentXformInGround),
            glm::value_ptr(deltaInGround),
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
        glm::vec3 translationInGround{};
        glm::vec3 rotationInGround{};
        glm::vec3 scaleInGround{};
        ImGuizmo::DecomposeMatrixToComponents(
            glm::value_ptr(deltaInGround),
            glm::value_ptr(translationInGround),
            glm::value_ptr(rotationInGround),
            glm::value_ptr(scaleInGround)
        );
        rotationInGround = glm::radians(rotationInGround);

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
        osc::PolarPerspectiveCamera const& camera,
        osc::Rect const& viewportRect,
        ImGuizmo::OPERATION operation,
        ImGuizmo::MODE mode,
        std::shared_ptr<osc::UndoableModelStatePair> model,
        OpenSim::Component const& selected,
        bool& wasUsingLastFrameStorage)
    {
        // try to downcast the selection as a station
        //
        // todo: OpenSim::PathPoint, etc.
        if (OpenSim::Station const* const maybeStation = dynamic_cast<OpenSim::Station const*>(&selected))
        {
            StationManipulator manipulator{model, *maybeStation};
            DrawGizmoOverlayInner(gizmoID, camera, viewportRect, operation, mode, manipulator, wasUsingLastFrameStorage);
        }
        else if (OpenSim::PathPoint const* const maybePathPoint = dynamic_cast<OpenSim::PathPoint const*>(&selected))
        {
            PathPointManipulator manipulator{model, *maybePathPoint};
            DrawGizmoOverlayInner(gizmoID, camera, viewportRect, operation, mode, manipulator, wasUsingLastFrameStorage);
        }
        else if (OpenSim::PhysicalOffsetFrame const* const maybePof = dynamic_cast<OpenSim::PhysicalOffsetFrame const*>(&selected))
        {
            PhysicalOffsetFrameManipulator manipulator{model, *maybePof};
            DrawGizmoOverlayInner(gizmoID, camera, viewportRect, operation, mode, manipulator, wasUsingLastFrameStorage);
        }
        else if (OpenSim::WrapObject const* const maybeWrapObject = dynamic_cast<OpenSim::WrapObject const*>(&selected))
        {
            WrapObjectManipulator manipulator{model, *maybeWrapObject};
            DrawGizmoOverlayInner(gizmoID, camera, viewportRect, operation, mode, manipulator, wasUsingLastFrameStorage);
        }
        else if (OpenSim::ContactGeometry const* const maybeContactGeom = dynamic_cast<OpenSim::ContactGeometry const*>(&selected))
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
    return ImGuizmo::IsUsing();
}

bool osc::ModelSelectionGizmo::handleKeyboardInputs()
{
    return osc::UpdateImguizmoStateFromKeyboard(m_GizmoOperation, m_GizmoMode);
}

void osc::ModelSelectionGizmo::draw(
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
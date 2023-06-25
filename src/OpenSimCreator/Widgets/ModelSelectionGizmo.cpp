#include "ModelSelectionGizmo.hpp"

#include "OpenSimCreator/Bindings/SimTKHelpers.hpp"
#include "OpenSimCreator/Model/UndoableModelStatePair.hpp"
#include "OpenSimCreator/Utils/OpenSimHelpers.hpp"
#include "OpenSimCreator/Utils/UndoableModelActions.hpp"

#include <oscar/Bindings/ImGuizmoHelpers.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/PolarPerspectiveCamera.hpp>
#include <oscar/Maths/Rect.hpp>
#include <oscar/Utils/Assertions.hpp>
#include <oscar/Utils/ScopeGuard.hpp>

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

// common/virtual manipulator data/APIs
namespace
{
    // operations that are supported by a manipulator
    using SupportedManipulationOpFlags = int32_t;
    enum SupportedManipulationOpFlags_ : int32_t {
        SupportedManipulationOpFlags_None = 0,
        SupportedManipulationOpFlags_Translation = 1<<0,
        SupportedManipulationOpFlags_Rotation = 1<<1,
    };

    // type-erased virtual base class that each concrete manipulator inherits from
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

        void onApplyRotation(glm::vec3 const& deltaEulerRadiansInGround)
        {
            implOnApplyRotation(deltaEulerRadiansInGround);
        }

        void onSave()
        {
            implOnSave();
        }
    private:
        virtual SupportedManipulationOpFlags implGetSupportedManipulationOps() const = 0;
        virtual glm::mat4 implGetCurrentModelMatrix() const = 0;
        virtual void implOnApplyTranslation(glm::vec3 const&) {}  // default to noop
        virtual void implOnApplyRotation(glm::vec3 const&) {}  // default to noop
        virtual void implOnSave() = 0;
    };

    // concrete implementation of a selection manipulator for `TComponent`
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
            OSC_ASSERT(m_Model != nullptr);
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
        // perform runtime lookup for `TComponent` and forward into concrete implementation
        glm::mat4 implGetCurrentModelMatrix() const final
        {
            TComponent const* maybeSelected = findSelection();
            if (!maybeSelected)
            {
                return glm::mat4{1.0f};  // selection of that type does not exist in the model
            }
            return implGetCurrentModelMatrix(*maybeSelected);
        }

        // perform runtime lookup for `TComponent` and forward into concrete implementation
        void implOnApplyTranslation(glm::vec3 const& deltaTranslationInGround) final
        {
            TComponent const* maybeSelected = findSelection();
            if (!maybeSelected)
            {
                return;  // selection of that type does not exist in the model
            }
            implOnApplyTranslation(*maybeSelected, deltaTranslationInGround);
        }

        // perform runtime lookup for `TComponent` and forward into concrete implementation
        void implOnApplyRotation(glm::vec3 const& deltaEulerRadiansInGround) final
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
        virtual glm::mat4 implGetCurrentModelMatrix(TComponent const&) const = 0;
        virtual void implOnApplyTranslation(TComponent const&, glm::vec3 const& deltaTranslationInGround) = 0;
        virtual void implOnApplyRotation(TComponent const&, glm::vec3 const&) {}  // default to noop
        virtual void implOnSave(TComponent const&) = 0;

        std::shared_ptr<osc::UndoableModelStatePair> m_Model;
        OpenSim::ComponentPath m_ComponentAbsPath;
    };
}

// concrete manipulator implementations
namespace
{
    // manipulator for `OpenSim::Station`
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

        glm::mat4 implGetCurrentModelMatrix(
            OpenSim::Station const& station) const final
        {
            SimTK::State const& state = getState();
            glm::mat4 currentXformInGround = osc::ToMat4(station.getParentFrame().getRotationInGround(state));
            currentXformInGround[3] = glm::vec4{osc::ToVec3(station.getLocationInGround(state)), 1.0f};

            return currentXformInGround;
        }

        void implOnApplyTranslation(
            OpenSim::Station const& station,
            glm::vec3 const& deltaTranslationInGround) final
        {
            SimTK::Rotation const parentToGroundRotation = station.getParentFrame().getRotationInGround(getState());
            SimTK::InverseRotation const& groundToParentRotation = parentToGroundRotation.invert();
            glm::vec3 const translationInParent = osc::ToVec3(groundToParentRotation * osc::ToSimTKVec3(deltaTranslationInGround));

            ActionTranslateStation(getUndoableModel(), station, translationInParent);
        }

        void implOnSave(OpenSim::Station const& station) final
        {
            ActionTranslateStationAndSave(getUndoableModel(), station, {});
        }
    };

    // manipulator for `OpenSim::PathPoint`
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

        glm::mat4 implGetCurrentModelMatrix(
            OpenSim::PathPoint const& pathPoint) const final
        {
            SimTK::State const& state = getState();
            glm::mat4 currentXformInGround = osc::ToMat4(pathPoint.getParentFrame().getRotationInGround(state));
            currentXformInGround[3] = glm::vec4{osc::ToVec3(pathPoint.getLocationInGround(state)), 1.0f};

            return currentXformInGround;
        }

        void implOnApplyTranslation(
            OpenSim::PathPoint const& pathPoint,
            glm::vec3 const& deltaTranslationInGround) final
        {
            SimTK::Rotation const parentToGroundRotation = pathPoint.getParentFrame().getRotationInGround(getState());
            SimTK::InverseRotation const& groundToParentRotation = parentToGroundRotation.invert();
            glm::vec3 const translationInParent = osc::ToVec3(groundToParentRotation * osc::ToSimTKVec3(deltaTranslationInGround));

            ActionTranslatePathPoint(getUndoableModel(), pathPoint, translationInParent);
        }

        void implOnSave(OpenSim::PathPoint const& pathPoint) final
        {
            ActionTranslatePathPointAndSave(getUndoableModel(), pathPoint, {});
        }
    };

    // manipulator for `OpenSim::PhysicalOffsetFrame`
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

        glm::mat4 implGetCurrentModelMatrix(
            OpenSim::PhysicalOffsetFrame const& pof) const final
        {
            return osc::ToMat4x4(pof.getTransformInGround(getState()));
        }

        void implOnApplyTranslation(
            OpenSim::PhysicalOffsetFrame const& pof,
            glm::vec3 const& deltaTranslationInGround) final
        {
            SimTK::Rotation const parentToGroundRotation = pof.getParentFrame().getRotationInGround(getState());
            SimTK::InverseRotation const& groundToParentRotation = parentToGroundRotation.invert();
            SimTK::Vec3 const deltaTranslationInParent = groundToParentRotation * osc::ToSimTKVec3(deltaTranslationInGround);
            SimTK::Vec3 const eulersInPofFrame = pof.get_orientation();

            ActionTransformPof(
                getUndoableModel(),
                pof,
                osc::ToVec3(deltaTranslationInParent),
                osc::ToVec3(eulersInPofFrame)
            );
        }

        void implOnApplyRotation(
            OpenSim::PhysicalOffsetFrame const& pof,
            glm::vec3 const& deltaEulerRadiansInGround) final
        {
            OpenSim::Frame const& parent = pof.getParentFrame();
            SimTK::State const& state = getState();

            glm::quat const deltaRotationInGround{deltaEulerRadiansInGround};
            glm::quat const oldRotationInGround{osc::ToQuat(pof.getRotationInGround(state))};
            glm::quat const parentRotationInGround = osc::ToQuat(parent.getRotationInGround(state));
            glm::quat const newRotationInGround = glm::normalize(deltaRotationInGround * oldRotationInGround);
            glm::quat const newRotationInParent = glm::inverse(parentRotationInGround) * newRotationInGround;

            ActionTransformPof(
                getUndoableModel(),
                pof,
                glm::vec3{},  // no translation delta
                osc::ExtractEulerAngleXYZ(newRotationInParent)
            );
        }

        void implOnSave(OpenSim::PhysicalOffsetFrame const& pof) final
        {
            std::stringstream ss;
            ss << "transformed " << pof.getName();
            getUndoableModel().commit(std::move(ss).str());
        }
    };

    // manipulator for `OpenSim::WrapObject`
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

        glm::mat4 implGetCurrentModelMatrix(
            OpenSim::WrapObject const& wrapObj) const final
        {
            SimTK::Transform const wrapToFrame = wrapObj.getTransform();
            SimTK::Transform const frameToGround = wrapObj.getFrame().getTransformInGround(getState());
            SimTK::Transform const wrapToGround = frameToGround * wrapToFrame;

            return osc::ToMat4x4(wrapToGround);
        }

        void implOnApplyTranslation(
            OpenSim::WrapObject const& wrapObj,
            glm::vec3 const& deltaTranslationInGround) final
        {
            SimTK::Rotation const frameToGroundRotation = wrapObj.getFrame().getTransformInGround(getState()).R();
            SimTK::InverseRotation const& groundToFrameRotation = frameToGroundRotation.invert();
            glm::vec3 const translationInPofFrame = osc::ToVec3(groundToFrameRotation * osc::ToSimTKVec3(deltaTranslationInGround));

            ActionTransformWrapObject(
                getUndoableModel(),
                wrapObj,
                translationInPofFrame,
                osc::ToVec3(wrapObj.get_xyz_body_rotation())
            );
        }

        void implOnApplyRotation(
            OpenSim::WrapObject const& wrapObj,
            glm::vec3 const& deltaEulerRadiansInGround) final
        {
            OpenSim::Frame const& parent = wrapObj.getFrame();
            SimTK::State const& state = getState();

            glm::quat const deltaRotationInGround{deltaEulerRadiansInGround};
            glm::quat const oldRotationInGround{osc::ToQuat(parent.getTransformInGround(state).R() * wrapObj.getTransform().R())};
            glm::quat const parentRotationInGround = osc::ToQuat(parent.getRotationInGround(state));
            glm::quat const newRotationInGround = glm::normalize(deltaRotationInGround * oldRotationInGround);
            glm::quat const newRotationInParent = glm::inverse(parentRotationInGround) * newRotationInGround;

            ActionTransformWrapObject(
                getUndoableModel(),
                wrapObj,
                glm::vec3{},  // no translation
                osc::ExtractEulerAngleXYZ(newRotationInParent)
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

        glm::mat4 implGetCurrentModelMatrix(
            OpenSim::ContactGeometry const& contactGeom) const final
        {
            SimTK::Transform const wrapToFrame = contactGeom.getTransform();
            SimTK::Transform const frameToGround = contactGeom.getFrame().getTransformInGround(getState());
            SimTK::Transform const wrapToGround = frameToGround * wrapToFrame;

            return osc::ToMat4x4(wrapToGround);
        }

        void implOnApplyTranslation(
            OpenSim::ContactGeometry const& contactGeom,
            glm::vec3 const& deltaTranslationInGround) final
        {
            SimTK::Rotation const frameToGroundRotation = contactGeom.getFrame().getTransformInGround(getState()).R();
            SimTK::InverseRotation const& groundToFrameRotation = frameToGroundRotation.invert();
            glm::vec3 const translationInPofFrame = osc::ToVec3(groundToFrameRotation * osc::ToSimTKVec3(deltaTranslationInGround));

            osc::ActionTransformContactGeometry(
                getUndoableModel(),
                contactGeom,
                translationInPofFrame,
                osc::ToVec3(contactGeom.get_orientation())
            );
        }

        void implOnApplyRotation(
            OpenSim::ContactGeometry const& contactGeom,
            glm::vec3 const& deltaEulerRadiansInGround) final
        {
            OpenSim::Frame const& parent = contactGeom.getFrame();
            SimTK::State const& state = getState();

            glm::quat const deltaRotationInGround{deltaEulerRadiansInGround};
            glm::quat const oldRotationInGround{osc::ToQuat(parent.getTransformInGround(state).R() * contactGeom.getTransform().R())};
            glm::quat const parentRotationInGround = osc::ToQuat(parent.getRotationInGround(state));
            glm::quat const newRotationInGround = glm::normalize(deltaRotationInGround * oldRotationInGround);
            glm::quat const newRotationInParent = glm::inverse(parentRotationInGround) * newRotationInGround;

            osc::ActionTransformContactGeometry(
                getUndoableModel(),
                contactGeom,
                glm::vec3{},  // no translation
                osc::ExtractEulerAngleXYZ(newRotationInParent)
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

        // important: necessary for multi-viewport gizmos
        // also important: don't use ImGui::GetID(), because it uses an ID stack and we might want to know if "isover" etc. is true outside of a window
        ImGuizmo::SetID(static_cast<int>(std::hash<void*>{}(gizmoID)));
        OSC_SCOPE_GUARD({ ImGuizmo::SetID(-1); });

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

        osc::SetImguizmoStyleToOSCStandard();
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
        // use downcasting to figure out which gizmo implementation to use
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
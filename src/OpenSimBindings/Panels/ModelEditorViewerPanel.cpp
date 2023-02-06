#include "ModelEditorViewerPanel.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Bindings/ImGuizmoHelpers.hpp"
#include "src/Graphics/IconCache.hpp"
#include "src/Graphics/MeshCache.hpp"
#include "src/Graphics/ShaderCache.hpp"
#include "src/Maths/MathHelpers.hpp"
#include "src/Maths/Rect.hpp"
#include "src/OpenSimBindings/MiddlewareAPIs/EditorAPI.hpp"
#include "src/OpenSimBindings/Rendering/CachedModelRenderer.hpp"
#include "src/OpenSimBindings/Rendering/ModelRendererParams.hpp"
#include "src/OpenSimBindings/Widgets/BasicWidgets.hpp"
#include "src/OpenSimBindings/Widgets/ComponentContextMenu.hpp"
#include "src/OpenSimBindings/ActionFunctions.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/SimTKHelpers.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Panels/StandardPanel.hpp"
#include "src/Platform/App.hpp"
#include "src/Widgets/GuiRuler.hpp"
#include "src/Widgets/IconWithoutMenu.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <ImGuizmo.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/Marker.h>
#include <OpenSim/Simulation/Model/PathPoint.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>

#include <memory>
#include <string_view>
#include <sstream>
#include <utility>

class osc::ModelEditorViewerPanel::Impl final : public osc::StandardPanel {
public:

    Impl(
        std::string_view panelName,
        MainUIStateAPI* mainUIStateAPI,
        EditorAPI* editorAPI,
        std::shared_ptr<UndoableModelStatePair> model) :

        StandardPanel{std::move(panelName)},
        m_MainUIStateAPI{mainUIStateAPI},
        m_EditorAPI{editorAPI},
        m_Model{std::move(model)}
    {
    }

private:
    void implBeforeImGuiBegin() final
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
    }

    void implAfterImGuiBegin() final
    {
        ImGui::PopStyleVar();
    }

    void implDrawContent() final
    {
        // compute viewer size (all available space)
        Rect const viewportRect = ContentRegionAvailScreenRect();

        // if this is the first frame being rendered, auto-focus the scene
        if (!m_MaybeLastHittest)
        {
            m_CachedModelRenderer.autoFocusCamera(
                *m_Model,
                m_Params,
                AspectRatio(viewportRect)
            );
        }

        // if hovering the panel, and not using an overlay, process mouse+keyboard inputs
        if (m_MaybeLastHittest &&
            m_MaybeLastHittest->isHovered &&
            !isUsingAnOverlay())
        {
            UpdatePolarCameraFromImGuiInputs(
                m_Params.camera,
                viewportRect,
                m_CachedModelRenderer.getRootAABB()
            );
        }

        // render the 3D scene to a texture and blit it via ImGui::Image
        {
            osc::RenderTexture& sceneTexture = m_CachedModelRenderer.draw(
                *m_Model,
                m_Params,
                Dimensions(viewportRect),
                App::get().getMSXAASamplesRecommended()
            );
            DrawTextureAsImGuiImage(sceneTexture, Dimensions(viewportRect));
        }

        // item-hittest the ImGui::Image so we know whether the user is interacting with it
        ImGuiItemHittestResult const imguiHittest = HittestLastImguiItem();
        m_MaybeLastHittest = imguiHittest;

        // if hovering the image item, and not dragging the mouse around, 3D-hittest the
        // scene so we know whether the user's mouse hits something in 3D
        std::optional<SceneCollision> maybeSceneCollision;
        if (imguiHittest.isHovered &&
            !IsDraggingWithAnyMouseButtonDown())
        {
            maybeSceneCollision = m_CachedModelRenderer.getClosestCollision(
                m_Params,
                ImGui::GetMousePos(),
                viewportRect
            );
        }

        // if the mouse hits something in 3D, and the mouse isn't busy doing something
        // else (e.g. dragging around) then lookup the 3D hit in the model, so we know
        // whether the user is interacting with a component in the model
        OpenSim::Component const* maybeHover = nullptr;
        if (maybeSceneCollision && !isUsingAnOverlay())
        {
            maybeHover = osc::FindComponent(
                m_Model->getModel(),
                maybeSceneCollision->decorationID
            );
        }

        // draw 2D overlays over the 3D scene image
        draw2DImguiOverlays(
            viewportRect,
            imguiHittest,
            maybeSceneCollision,
            maybeHover
        );

        // handle any other model/state mutations as a result of interaction
        handleInteractionRelatedModelSideEffects(
            imguiHittest,
            maybeHover
        );
    }

    bool isUsingAnOverlay() const
    {
        return m_Ruler.isMeasuring() || ImGuizmo::IsUsing();
    }

    // uses ImGui's 2D drawlist to draw interactive widgets/overlays on the panel
    void draw2DImguiOverlays(
        Rect const& viewportRect,
        ImGuiItemHittestResult const& imguiHittest,
        std::optional<SceneCollision> const& maybeSceneHittest,
        OpenSim::Component const* maybeHover)
    {
        // draw generic overlays (i.e. the buttons for toggling things)
        DrawViewerImGuiOverlays(
            m_Params,
            m_CachedModelRenderer.getDrawlist(),
            m_CachedModelRenderer.getRootAABB(),
            viewportRect,
            *m_IconCache,
            [this]() { drawExtraTopButtons(); }
        );

        // if applicable, draw the ruler
        m_Ruler.draw(m_Params.camera, viewportRect, maybeSceneHittest);

        // draw gizmo manipulators over the top
        drawGizmoOverlay(viewportRect);

        if (maybeHover)
        {
            DrawComponentHoverTooltip(*maybeHover);
        }

        // right-click: open context menu
        if (imguiHittest.isRightClickReleasedWithoutDragging)
        {
            // right-click: draw a context menu
            std::string const menuName = std::string{getName()} + "_contextmenu";
            OpenSim::ComponentPath const path = osc::GetAbsolutePathOrEmpty(maybeHover);
            m_EditorAPI->pushPopup(std::make_unique<ComponentContextMenu>(menuName, m_MainUIStateAPI, m_EditorAPI, m_Model, path));
        }
    }

    // draws extra top overlay buttons
    void drawExtraTopButtons()
    {
        IconWithoutMenu rulerButton
        {
            m_IconCache->getIcon("ruler"),
            "Ruler",
            "Roughly measure something in the scene",
        };
        if (rulerButton.draw())
        {
            m_Ruler.toggleMeasuring();
        }
        ImGui::SameLine();

        DrawGizmoOpSelector(m_GizmoOperation, true, true, false);  // translate/rotate selector
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0f, 0.0f});
        ImGui::SameLine();
        ImGui::PopStyleVar();
        DrawGizmoModeSelector(m_GizmoMode); // global/world selector
    }

    // draws 3D manipulation gizmo overlay
    void drawGizmoOverlay(Rect const& viewportRect)
    {
        // get selection as a component
        OpenSim::Component const* const maybeSelected = m_Model->getSelected();
        if (!maybeSelected)
        {
            return;  // nothing selected (don't draw gizmos)
        }
        OpenSim::Component const& selected = *maybeSelected;

        // try to downcast the selection as a station
        //
        // todo: OpenSim::PathPoint, etc.
        if (OpenSim::Station const* const maybeStation = dynamic_cast<OpenSim::Station const*>(&selected))
        {
            drawGizmoOverlayForStation(viewportRect, *maybeStation);
        }
        else if (OpenSim::PathPoint const* const maybePathPoint = dynamic_cast<OpenSim::PathPoint const*>(&selected))
        {
            drawGizmoOverlayForUserPathPoint(viewportRect, *maybePathPoint);
        }
        else if (OpenSim::PhysicalOffsetFrame const* const maybePof = dynamic_cast<OpenSim::PhysicalOffsetFrame const*>(&selected))
        {
            drawGizmoOverlayForPhysicalOffsetFrame(viewportRect, *maybePof);
        }
        else
        {
            // (do nothing: we don't know how to manipulate the selection0
        }
    }

    void setupImguizmo(Rect const& viewportRect)
    {
        ImGuizmo::SetID(ImGui::GetID(this));  // important: necessary for multi-viewport gizmos
        ImGuizmo::SetRect(
            viewportRect.p1.x,
            viewportRect.p1.y,
            Dimensions(viewportRect).x,
            Dimensions(viewportRect).y
        );
        ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
        ImGuizmo::AllowAxisFlip(false);
    }

    // HACK: draw gizmos for station
    //
    // (it's a hack because this should be abstracted, rather than copy-pasted for each
    //  manipulation type)
    void drawGizmoOverlayForStation(Rect const& viewportRect, OpenSim::Station const& station)
    {
        if (m_GizmoOperation != ImGuizmo::TRANSLATE)
        {
            return;  // can only translate path stations
        }

        setupImguizmo(viewportRect);

        // use rotation from the parent, translation from station
        glm::mat4 currentXformInGround = ToMat4(station.getParentFrame().getRotationInGround(m_Model->getState()));
        currentXformInGround[3] = glm::vec4{ToVec3(station.getLocationInGround(m_Model->getState())), 1.0f};
        glm::mat4 deltaInGround;

        bool const gizmoWasManipulatedByUser = ImGuizmo::Manipulate(
            glm::value_ptr(m_Params.camera.getViewMtx()),
            glm::value_ptr(m_Params.camera.getProjMtx(AspectRatio(viewportRect))),
            m_GizmoOperation,
            m_GizmoMode,
            glm::value_ptr(currentXformInGround),
            glm::value_ptr(deltaInGround),
            nullptr,
            nullptr,
            nullptr
        );

        bool const isUsingThisFrame = ImGuizmo::IsUsing();
        bool const wasUsingLastFrame = m_WasUsingGizmoLastFrame;
        m_WasUsingGizmoLastFrame = isUsingThisFrame;  // update cached state

        if (wasUsingLastFrame && !isUsingThisFrame)
        {
            // user finished interacting, save model
            ActionTranslateStationAndSave(*m_Model, station, {});
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

        // apply transformation to component in-place (but don't save - would be very slow)
        SimTK::Rotation const parentToGroundRotation = station.getParentFrame().getRotationInGround(m_Model->getState());
        SimTK::InverseRotation const& groundToParentRotation = parentToGroundRotation.invert();
        glm::vec3 const translationInParent = ToVec3(groundToParentRotation * ToSimTKVec3(translationInGround));
        ActionTranslateStation(*m_Model, station, translationInParent);
    }

    void drawGizmoOverlayForUserPathPoint(
        Rect const& viewportRect,
        OpenSim::PathPoint const& pathPoint)
    {
        if (m_GizmoOperation != ImGuizmo::TRANSLATE)
        {
            return;  // can only translate path points
        }

        setupImguizmo(viewportRect);

        // use rotation from the parent, translation from station
        glm::mat4 currentXformInGround = ToMat4(pathPoint.getParentFrame().getRotationInGround(m_Model->getState()));
        currentXformInGround[3] = glm::vec4{ToVec3(pathPoint.getLocationInGround(m_Model->getState())), 1.0f};
        glm::mat4 deltaInGround;

        bool const gizmoWasManipulatedByUser = ImGuizmo::Manipulate(
            glm::value_ptr(m_Params.camera.getViewMtx()),
            glm::value_ptr(m_Params.camera.getProjMtx(AspectRatio(viewportRect))),
            m_GizmoOperation,
            m_GizmoMode,
            glm::value_ptr(currentXformInGround),
            glm::value_ptr(deltaInGround),
            nullptr,
            nullptr,
            nullptr
        );
        bool const isUsingThisFrame = ImGuizmo::IsUsing();
        bool const wasUsingLastFrame = m_WasUsingGizmoLastFrame;
        m_WasUsingGizmoLastFrame = isUsingThisFrame;  // update cached state

        if (wasUsingLastFrame && !isUsingThisFrame)
        {
            // user finished interacting, save model
            ActionTranslatePathPointAndSave(*m_Model, pathPoint, {});
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

        // apply transformation to component in-place (but don't save - would be very slow)
        SimTK::Rotation const parentToGroundRotation = pathPoint.getParentFrame().getRotationInGround(m_Model->getState());
        SimTK::InverseRotation const& groundToParentRotation = parentToGroundRotation.invert();
        glm::vec3 const translationInParent = ToVec3(groundToParentRotation * ToSimTKVec3(translationInGround));
        ActionTranslatePathPoint(*m_Model, pathPoint, translationInParent);
    }

    void drawGizmoOverlayForPhysicalOffsetFrame(
        Rect const& viewportRect,
        OpenSim::PhysicalOffsetFrame const& pof)
    {
        if (m_GizmoOperation != ImGuizmo::TRANSLATE &&
            m_GizmoOperation != ImGuizmo::ROTATE)
        {
            return;  // can only translate/rotate offset frames
        }

        setupImguizmo(viewportRect);

        // use the PoF's own rotation as its local space
        glm::mat4 currentXformInGround = ToMat4(pof.getRotationInGround(m_Model->getState()));
        currentXformInGround[3] = glm::vec4{ToVec3(pof.getPositionInGround(m_Model->getState())), 1.0f};
        glm::mat4 deltaInGround;

        bool const gizmoWasManipulatedByUser = ImGuizmo::Manipulate(
            glm::value_ptr(m_Params.camera.getViewMtx()),
            glm::value_ptr(m_Params.camera.getProjMtx(AspectRatio(viewportRect))),
            m_GizmoOperation,
            m_GizmoMode,
            glm::value_ptr(currentXformInGround),
            glm::value_ptr(deltaInGround),
            nullptr,
            nullptr,
            nullptr
        );
        bool const isUsingThisFrame = ImGuizmo::IsUsing();
        bool const wasUsingLastFrame = m_WasUsingGizmoLastFrame;
        m_WasUsingGizmoLastFrame = isUsingThisFrame;  // update cached state

        if (wasUsingLastFrame && !isUsingThisFrame)
        {
            std::stringstream ss;
            ss << "transformed " << pof.getName();
            m_Model->commit(std::move(ss).str());
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

        // apply transformation to component in-place (but don't save - would be very slow)
        glm::vec3 translationInPofFrame{};
        glm::vec3 eulersInPofFrame = {};
        if (m_GizmoOperation == ImGuizmo::TRANSLATE)
        {
            SimTK::Rotation const pofToGroundRotation =  pof.getRotationInGround(m_Model->getState());
            SimTK::InverseRotation const& groundToParentRotation = pofToGroundRotation.invert();
            translationInPofFrame = ToVec3(groundToParentRotation * ToSimTKVec3(translationInGround));
        }
        else if (m_GizmoOperation == ImGuizmo::ROTATE)
        {
            osc::Transform t = osc::ToTransform(pof.getTransformInGround(m_Model->getState()));
            ApplyWorldspaceRotation(t, rotationInGround, currentXformInGround[3]);
            eulersInPofFrame = ExtractEulerAngleXYZ(t);
        }

        ActionTransformPof(*m_Model, pof, translationInPofFrame, eulersInPofFrame);
    }

    // handles any interactions that change the model (e.g. what's selected)
    void handleInteractionRelatedModelSideEffects(
        ImGuiItemHittestResult const& imguiHittest,
        OpenSim::Component const* maybeHover)
    {
        // handle hover mutations
        if (isUsingAnOverlay())
        {
            m_Model->setHovered(nullptr);
        }
        else if (imguiHittest.isHovered && maybeHover != m_Model->getHovered())
        {
            // care: this code must check whether the hover != current hover
            // (even if null), because there might be multiple viewports open
            // (#582)
            m_Model->setHovered(maybeHover);
        }

        // left-click: set model selection to (potentially empty) hover
        if (imguiHittest.isLeftClickReleasedWithoutDragging &&
            !isUsingAnOverlay())
        {
            m_Model->setSelected(maybeHover);
        }
    }

    // tab/model state
    MainUIStateAPI* m_MainUIStateAPI;
    EditorAPI* m_EditorAPI;
    std::shared_ptr<UndoableModelStatePair> m_Model;

    // 3D render/image state
    ModelRendererParams m_Params;
    CachedModelRenderer m_CachedModelRenderer
    {
        App::get().getConfig(),
        App::singleton<MeshCache>(),
        *App::singleton<ShaderCache>(),
    };
    std::optional<ImGuiItemHittestResult> m_MaybeLastHittest;

    // overlay state
    std::shared_ptr<IconCache> m_IconCache = App::singleton<osc::IconCache>(
        App::resource("icons/"),
        ImGui::GetTextLineHeight()/128.0f
    );
    GuiRuler m_Ruler;
    bool m_WasUsingGizmoLastFrame = false;
    ImGuizmo::OPERATION m_GizmoOperation = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE m_GizmoMode = ImGuizmo::WORLD;
};


// public API (PIMPL)

osc::ModelEditorViewerPanel::ModelEditorViewerPanel(
    std::string_view panelName,
    MainUIStateAPI* mainUIStateAPI,
    EditorAPI* editorAPI,
    std::shared_ptr<UndoableModelStatePair> model) :

    m_Impl{std::make_unique<Impl>(std::move(panelName), mainUIStateAPI, editorAPI, std::move(model))}
{
}
osc::ModelEditorViewerPanel::ModelEditorViewerPanel(ModelEditorViewerPanel&&) noexcept = default;
osc::ModelEditorViewerPanel& osc::ModelEditorViewerPanel::operator=(ModelEditorViewerPanel&&) noexcept = default;
osc::ModelEditorViewerPanel::~ModelEditorViewerPanel() noexcept = default;

osc::CStringView osc::ModelEditorViewerPanel::implGetName() const
{
    return m_Impl->getName();
}

bool osc::ModelEditorViewerPanel::implIsOpen() const
{
    return m_Impl->isOpen();
}

void osc::ModelEditorViewerPanel::implOpen()
{
    m_Impl->open();
}

void osc::ModelEditorViewerPanel::implClose()
{
    m_Impl->close();
}

void osc::ModelEditorViewerPanel::implDraw()
{
    m_Impl->draw();
}
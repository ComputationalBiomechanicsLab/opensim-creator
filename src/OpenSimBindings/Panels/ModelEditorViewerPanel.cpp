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
#include "src/OpenSimBindings/Widgets/ModelSelectionGizmo.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Panels/StandardPanel.hpp"
#include "src/Platform/App.hpp"
#include "src/Widgets/GuiRuler.hpp"
#include "src/Widgets/IconWithoutMenu.hpp"

#include <imgui.h>
#include <ImGuizmo.h>
#include <OpenSim/Common/ComponentPath.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <sstream>
#include <utility>

class osc::ModelEditorViewerPanel::Impl final : public osc::StandardPanel {
public:

    Impl(
        std::string_view panelName_,
        std::weak_ptr<MainUIStateAPI> mainUIStateAPI_,
        EditorAPI* editorAPI_,
        std::shared_ptr<UndoableModelStatePair> model_) :

        StandardPanel{std::move(panelName_)},
        m_MainUIStateAPI{std::move(mainUIStateAPI_)},
        m_EditorAPI{std::move(editorAPI_)},
        m_Model{std::move(model_)}
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
            handleMouseAndKeyboardInputs(viewportRect);
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
        return m_Ruler.isMeasuring() || m_Gizmo.isUsing();
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
        m_Gizmo.draw(viewportRect, m_Params.camera);

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

        // draw translate/rotate/scale selector
        {
            ImGuizmo::OPERATION op = m_Gizmo.getOperation();
            if (DrawGizmoOpSelector(op, true, true, false))
            {
                m_Gizmo.setOperation(op);
            }
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0f, 0.0f});
        ImGui::SameLine();
        ImGui::PopStyleVar();

        // draw global/world selector
        {
            ImGuizmo::MODE mode = m_Gizmo.getMode();
            if (DrawGizmoModeSelector(mode))
            {
                m_Gizmo.setMode(mode);
            }
        }
    }

    bool handleMouseAndKeyboardInputs(Rect const& viewportRect)
    {
        if (m_Gizmo.handleKeyboardInputs())
        {
            return true;
        }
        else if (UpdatePolarCameraFromImGuiInputs(m_Params.camera, viewportRect, m_CachedModelRenderer.getRootAABB()))
        {
            return true;
        }
        else
        {
            return false;
        }
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
    std::weak_ptr<MainUIStateAPI> m_MainUIStateAPI;
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
    ModelSelectionGizmo m_Gizmo{m_Model};
};


// public API (PIMPL)

osc::ModelEditorViewerPanel::ModelEditorViewerPanel(
    std::string_view panelName_,
    std::weak_ptr<MainUIStateAPI> mainUIStateAPI_,
    EditorAPI* editorAPI_,
    std::shared_ptr<UndoableModelStatePair> model_) :

    m_Impl{std::make_unique<Impl>(std::move(panelName_), std::move(mainUIStateAPI_), editorAPI_, std::move(model_))}
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
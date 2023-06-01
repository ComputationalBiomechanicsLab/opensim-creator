#include "ModelEditorViewerPanel.hpp"

#include "OpenSimCreator/Graphics/CachedModelRenderer.hpp"
#include "OpenSimCreator/Panels/ModelEditorViewerPanelLayer.hpp"
#include "OpenSimCreator/Panels/ModelEditorViewerPanelParameters.hpp"
#include "OpenSimCreator/Panels/ModelEditorViewerPanelRightClickEvent.hpp"
#include "OpenSimCreator/Panels/ModelEditorViewerPanelState.hpp"
#include "OpenSimCreator/Widgets/BasicWidgets.hpp"
#include "OpenSimCreator/Widgets/ModelSelectionGizmo.hpp"
#include "OpenSimCreator/OpenSimHelpers.hpp"
#include "OpenSimCreator/UndoableModelStatePair.hpp"

#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Bindings/ImGuizmoHelpers.hpp>
#include <oscar/Graphics/IconCache.hpp>
#include <oscar/Graphics/MeshCache.hpp>
#include <oscar/Graphics/ShaderCache.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Rect.hpp>
#include <oscar/Panels/StandardPanel.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/Utils/Algorithms.hpp>
#include <oscar/Widgets/GuiRuler.hpp>
#include <oscar/Widgets/IconWithoutMenu.hpp>

#include <imgui.h>
#include <ImGuizmo.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <sstream>
#include <utility>
#include <vector>

namespace
{
    class RulerLayer final : public osc::ModelEditorViewerPanelLayer {
    public:
        RulerLayer()
        {
            m_Ruler.startMeasuring();
        }

    private:
        void implOnDraw(
            osc::ModelEditorViewerPanelParameters& params,
            osc::ModelEditorViewerPanelState const& state) final
        {
            m_Ruler.draw(
                params.getRenderParams().camera,
                state.viewportRect,
                state.maybeBaseLayerHittest
            );
        }

        bool implShouldClose() const
        {
            return !m_Ruler.isMeasuring();
        }

        osc::GuiRuler m_Ruler;
    };
}

class osc::ModelEditorViewerPanel::Impl final : public osc::StandardPanel {
public:

    Impl(
        std::string_view panelName_,
        ModelEditorViewerPanelParameters const& parameters_) :

        StandardPanel{panelName_},
        m_Parameters{parameters_}
    {
    }

    ModelEditorViewerPanelLayer& pushLayer(std::unique_ptr<ModelEditorViewerPanelLayer> layer)
    {
        return *m_Layers.emplace_back(std::move(layer));
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
        // compute viewer size (fill the whole panel)
        Rect const viewportRect = ContentRegionAvailScreenRect();

        // if this is the first frame being rendered, auto-focus the scene
        if (!m_MaybeLastHittest)
        {
            m_CachedModelRenderer.autoFocusCamera(
                *m_Parameters.getModelSharedPtr(),
                m_Parameters.updRenderParams(),
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
                *m_Parameters.getModelSharedPtr(),
                m_Parameters.getRenderParams(),
                Dimensions(viewportRect),
                App::get().getMSXAASamplesRecommended()
            );
            DrawTextureAsImGuiImage(sceneTexture, Dimensions(viewportRect));
        }

        // item-hittest the ImGui::Image so we know whether the user is interacting with it
        ImGuiItemHittestResult imguiHittest = HittestLastImguiItem();
        imguiHittest.isHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

        m_MaybeLastHittest = imguiHittest;

        // if hovering the image item, and not dragging the mouse around, 3D-hittest the
        // scene so we know whether the user's mouse hits something in 3D
        std::optional<SceneCollision> maybeSceneCollision;
        if (imguiHittest.isHovered &&
            !IsDraggingWithAnyMouseButtonDown())
        {
            maybeSceneCollision = m_CachedModelRenderer.getClosestCollision(
                m_Parameters.getRenderParams(),
                ImGui::GetMousePos(),
                viewportRect
            );
        }

        // if the mouse hits something in 3D, and the mouse isn't busy using a gizmo,
        // then lookup the 3D hit in the model, so we know whether the user is
        // interacting with a component in the model
        OpenSim::Component const* maybeHover = nullptr;
        if (maybeSceneCollision && !m_Gizmo.isUsing() && !m_Gizmo.isOver())
        {
            maybeHover = osc::FindComponent(
                m_Parameters.getModelSharedPtr()->getModel(),
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

        // garbage-collect closed layers
        garbageCollectClosedLayers();
    }

    void garbageCollectClosedLayers()
    {
        osc::RemoveErase(m_Layers, [](auto const& layerPtr) { return layerPtr->shouldClose(); });
    }

    bool isUsingAnOverlay() const
    {
        return !m_Layers.empty() || m_Gizmo.isUsing();
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
            m_Parameters.updRenderParams(),
            m_CachedModelRenderer.getDrawlist(),
            m_CachedModelRenderer.getRootAABB(),
            viewportRect,
            *m_IconCache,
            [this]() { drawExtraTopButtons(); }
        );

        if (m_Layers.empty())
        {
            // draw gizmo manipulators over the top
            m_Gizmo.draw(viewportRect, m_Parameters.getRenderParams().camera);

            if (maybeHover)
            {
                DrawComponentHoverTooltip(*maybeHover);
            }

            // right-click: open context menu
            if (imguiHittest.isRightClickReleasedWithoutDragging)
            {
                // right-click: pump a right-click event
                ModelEditorViewerPanelRightClickEvent const e
                {
                    std::string{getName()},
                    viewportRect,
                    osc::GetAbsolutePathOrEmpty(maybeHover).toString(),
                    maybeSceneHittest ? std::optional<glm::vec3>{maybeSceneHittest->worldspaceLocation} : std::nullopt,
                };
                m_Parameters.callOnRightClickHandler(e);
            }
        }
        else
        {
            ModelEditorViewerPanelState state
            {
                viewportRect,
                maybeSceneHittest,
            };

            int id = 0;
            for (auto const& layerPtr : m_Layers)
            {
                ModelEditorViewerPanelLayerFlags const layerFlags = layerPtr->getFlags();
                ImGuiWindowFlags windowFlags = osc::GetMinimalWindowFlags();

                if (layerFlags & ModelEditorViewerPanelLayerFlags_CapturesInputs)
                {
                    windowFlags &= ~ImGuiWindowFlags_NoInputs;
                }

                windowFlags &= ~ImGuiWindowFlags_NoBackground;
                ImGui::SetNextWindowBgAlpha(layerPtr->getBackgroundAlpha());

                ImGui::SetNextWindowPos(viewportRect.p1);
                if (ImGui::BeginChild(id, Dimensions(viewportRect), false, windowFlags))
                {
                    layerPtr->onDraw(m_Parameters, state);
                    ImGui::EndChild();
                }
                ++id;
            }
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
            pushLayer(std::make_unique<RulerLayer>());
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
        else if (UpdatePolarCameraFromImGuiInputs(m_Parameters.updRenderParams().camera, viewportRect, m_CachedModelRenderer.getRootAABB()))
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
            m_Parameters.getModelSharedPtr()->setHovered(nullptr);
        }
        else if (imguiHittest.isHovered && maybeHover != m_Parameters.getModelSharedPtr()->getHovered())
        {
            // care: this code must check whether the hover != current hover
            // (even if null), because there might be multiple viewports open
            // (#582)
            m_Parameters.getModelSharedPtr()->setHovered(maybeHover);
        }

        // left-click: set model selection to (potentially empty) hover
        if (imguiHittest.isLeftClickReleasedWithoutDragging &&
            !isUsingAnOverlay())
        {
            m_Parameters.getModelSharedPtr()->setSelected(maybeHover);
        }
    }

    ModelEditorViewerPanelParameters m_Parameters;
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
    std::vector<std::unique_ptr<ModelEditorViewerPanelLayer>> m_Layers;
    ModelSelectionGizmo m_Gizmo{m_Parameters.getModelSharedPtr()};
};


// public API (PIMPL)

osc::ModelEditorViewerPanel::ModelEditorViewerPanel(
    std::string_view panelName_,
    ModelEditorViewerPanelParameters const& parameters_) :

    m_Impl{std::make_unique<Impl>(panelName_, parameters_)}
{
}

osc::ModelEditorViewerPanel::ModelEditorViewerPanel(ModelEditorViewerPanel&&) noexcept = default;
osc::ModelEditorViewerPanel& osc::ModelEditorViewerPanel::operator=(ModelEditorViewerPanel&&) noexcept = default;
osc::ModelEditorViewerPanel::~ModelEditorViewerPanel() noexcept = default;

osc::ModelEditorViewerPanelLayer& osc::ModelEditorViewerPanel::pushLayer(std::unique_ptr<ModelEditorViewerPanelLayer> layer)
{
    return m_Impl->pushLayer(std::move(layer));
}

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
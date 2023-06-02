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
        osc::ModelEditorViewerPanelLayerFlags implGetFlags() const final
        {
            return osc::ModelEditorViewerPanelLayerFlags_CapturesMouseInputs;
        }

        bool implHandleMouseInputs()
        {
            return true;  // always handles the mouse
        }

        void implOnDraw(
            osc::ModelEditorViewerPanelParameters& params,
            osc::ModelEditorViewerPanelState& state) final
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

    // model viewer layer that adds buttons for controling visualization
    // options and 3D manipulator gizmos
    class ButtonAndGizmoControlsLayer final : public osc::ModelEditorViewerPanelLayer {
    public:
        explicit ButtonAndGizmoControlsLayer(std::shared_ptr<osc::UndoableModelStatePair> model_) :
            m_Gizmo{model_}
        {
        }
    private:
        osc::ModelEditorViewerPanelLayerFlags implGetFlags() const final
        {
            osc::ModelEditorViewerPanelLayerFlags flags = osc::ModelEditorViewerPanelLayerFlags_None;
            if (m_Gizmo.isUsing())
            {
                flags |= osc::ModelEditorViewerPanelLayerFlags_CapturesMouseInputs;
            }
            return flags;
        }

        float implGetBackgroundAlpha() const final
        {
            return 0.0f;
        }

        bool implHandleMouseInputs(
            osc::ModelEditorViewerPanelParameters&,
            osc::ModelEditorViewerPanelState&)
        {
            if (m_Gizmo.isUsing())
            {
                return true;
            }

            // care: ImGuizmo::isOver can return `true` even if it
            // isn't being drawn this frame

            return false;
        }

        bool implHandleKeyboardInputs(
            osc::ModelEditorViewerPanelParameters& params,
            osc::ModelEditorViewerPanelState& state) final
        {
            return m_Gizmo.handleKeyboardInputs();
        }

        void implOnDraw(
            osc::ModelEditorViewerPanelParameters& params,
            osc::ModelEditorViewerPanelState& state) final
        {
            // draw generic overlays (i.e. the buttons for toggling things)
            DrawViewerImGuiOverlays(
                params.updRenderParams(),
                state.getDrawlist(),
                state.maybeSceneAABB,
                state.viewportRect,
                *m_IconCache,
                [this, &state]() { drawExtraTopButtons(state); }
            );

            // draw gizmo manipulators over the top
            m_Gizmo.draw(state.viewportRect, params.getRenderParams().camera);
        }

        bool implShouldClose() const final
        {
            return false;  // never closes
        }

        // draws extra top overlay buttons
        void drawExtraTopButtons(
            osc::ModelEditorViewerPanelState& state)
        {
            osc::IconWithoutMenu rulerButton
            {
                m_IconCache->getIcon("ruler"),
                "Ruler",
                "Roughly measure something in the scene",
            };
            if (rulerButton.draw())
            {
                state.pushLayer(std::make_unique<RulerLayer>());
            }
            ImGui::SameLine();

            // draw translate/rotate/scale selector
            {
                ImGuizmo::OPERATION op = m_Gizmo.getOperation();
                if (osc::DrawGizmoOpSelector(op, true, true, false))
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
                if (osc::DrawGizmoModeSelector(mode))
                {
                    m_Gizmo.setMode(mode);
                }
            }
        }

        // buttons + gizmo layer state
        std::shared_ptr<osc::IconCache> m_IconCache = osc::App::singleton<osc::IconCache>(
            osc::App::resource("icons/"),
            ImGui::GetTextLineHeight()/128.0f
        );
        osc::ModelSelectionGizmo m_Gizmo;
    };

    // the "base" model viewer layer, which is the last layer to handle any input
    // etc. if no upper layer decides to handle it
    class BaseInteractionLayer final : public osc::ModelEditorViewerPanelLayer {
    private:
        void implOnNewFrame() final
        {
            m_IsHandlingMouseInputs = false;

        }

        bool implHandleKeyboardInputs(
            osc::ModelEditorViewerPanelParameters& params,
            osc::ModelEditorViewerPanelState& state) final
        {
            return osc::UpdatePolarCameraFromImGuiKeyboardInputs(
                params.updRenderParams().camera,
                state.viewportRect,
                state.maybeSceneAABB
            );
        }

        bool implHandleMouseInputs(
            osc::ModelEditorViewerPanelParameters& params,
            osc::ModelEditorViewerPanelState& state) final
        {
            m_IsHandlingMouseInputs = true;

            // try updating the camera (mouse panning, etc.)
            bool rv = osc::UpdatePolarCameraFromImGuiMouseInputs(
                osc::Dimensions(state.viewportRect),
                params.updRenderParams().camera
            );

            if (osc::IsDraggingWithAnyMouseButtonDown())
            {
                params.getModelSharedPtr()->setHovered(nullptr);
            }
            else if (state.maybeHoveredComponent != params.getModelSharedPtr()->getHovered())
            {
                // care: this code must check whether the hover != current hover
                // (even if null), because there might be multiple viewports open
                // (#582)
                params.getModelSharedPtr()->setHovered(state.maybeHoveredComponent);
                rv = true;
            }

            // if left-clicked, update top-level model selection
            if (state.isLeftClickReleasedWithoutDragging)
            {
                params.getModelSharedPtr()->setSelected(state.maybeHoveredComponent);
                rv = true;
            }

            return rv;
        }

        void implOnDraw(
            osc::ModelEditorViewerPanelParameters& params,
            osc::ModelEditorViewerPanelState& state) final
        {
            // hover, but not panning: show tooltip
            if (state.maybeHoveredComponent &&
                m_IsHandlingMouseInputs &&
                !osc::IsDraggingWithAnyMouseButtonDown())
            {
                osc::DrawComponentHoverTooltip(*state.maybeHoveredComponent);
            }

            // right-click: open context menu
            if (m_IsHandlingMouseInputs &&
                state.isRightClickReleasedWithoutDragging)
            {
                // right-click: pump a right-click event
                osc::ModelEditorViewerPanelRightClickEvent const e
                {
                    std::string{state.getPanelName()},
                    state.viewportRect,
                    osc::GetAbsolutePathOrEmpty(state.maybeHoveredComponent).toString(),
                    state.maybeBaseLayerHittest ? std::optional<glm::vec3>{state.maybeBaseLayerHittest->worldspaceLocation} : std::nullopt,
                };
                params.callOnRightClickHandler(e);
            }
        }

        bool implShouldClose() const final
        {
            return false;
        }

        bool m_IsHandlingMouseInputs = false;
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
        pushLayer(std::make_unique<BaseInteractionLayer>());
        pushLayer(std::make_unique<ButtonAndGizmoControlsLayer>(m_Parameters.getModelSharedPtr()));
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
        bool const isHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

        m_State.viewportRect = ContentRegionAvailScreenRect();
        m_State.isLeftClickReleasedWithoutDragging = IsMouseReleasedWithoutDragging(ImGuiMouseButton_Left);
        m_State.isRightClickReleasedWithoutDragging = IsMouseReleasedWithoutDragging(ImGuiMouseButton_Right);

        // if necessary, auto-focus the camera on the first frame
        if (m_IsFirstFrame)
        {
            m_State.updRenderer().autoFocusCamera(
                *m_Parameters.getModelSharedPtr(),
                m_Parameters.updRenderParams(),
                AspectRatio(m_State.viewportRect)
            );
            m_IsFirstFrame = false;
        }

        layersOnNewFrame();

        // if the viewer is hovered, handle inputs
        if (isHovered)
        {
            layersHandleMouseInputs();
            layersHandleKeyboardInputs();
        }

        // render the 3D scene to a texture and present it via an ImGui::Image
        {
            osc::RenderTexture& sceneTexture = m_State.updRenderer().draw(
                *m_Parameters.getModelSharedPtr(),
                m_Parameters.getRenderParams(),
                Dimensions(m_State.viewportRect),
                App::get().getMSXAASamplesRecommended()
            );
            DrawTextureAsImGuiImage(
                sceneTexture,
                Dimensions(m_State.viewportRect)
            );
        }

        // update state scene AABB
        m_State.maybeSceneAABB = m_State.getRenderer().getRootAABB();

        // if hovering in 2D, 3D-hittest the scene
        if (isHovered)
        {
            m_State.maybeBaseLayerHittest = m_State.getRenderer().getClosestCollision(
                m_Parameters.getRenderParams(),
                ImGui::GetMousePos(),
                m_State.viewportRect
            );
        }
        else
        {
            m_State.maybeBaseLayerHittest.reset();
        }

        // if there's a 3D-hit, transform it into an OpenSim-hit
        if (m_State.maybeBaseLayerHittest)
        {
            m_State.maybeHoveredComponent = osc::FindComponent(
                m_Parameters.getModelSharedPtr()->getModel(),
                m_State.maybeBaseLayerHittest->decorationID
            );
        }
        else
        {
            m_State.maybeHoveredComponent = nullptr;
        }

        layersDraw();
        layersGarbageCollect();
        layersPopQueuedNewLayers();
    }

    void layersOnNewFrame()
    {
        for (auto const& layerPtr : m_Layers)
        {
            layerPtr->onNewFrame();
        }
    }

    void layersHandleKeyboardInputs()
    {
        for (auto it = m_Layers.rbegin(); it != m_Layers.rend(); ++it)
        {
            if ((*it)->handleKeyboardInputs(m_Parameters, m_State))
            {
                return;
            }
        }
    }

    void layersHandleMouseInputs()
    {
        for (auto it = m_Layers.rbegin(); it != m_Layers.rend(); ++it)
        {
            if ((*it)->handleMouseInputs(m_Parameters, m_State) ||
                (*it)->getFlags() & ModelEditorViewerPanelLayerFlags_CapturesMouseInputs)
            {
                return;
            }
        }
    }

    void layersDraw()
    {
        int childWindowID = 0;
        for (auto it = m_Layers.begin(); it != m_Layers.end(); ++it)
        {
            ModelEditorViewerPanelLayer& layer = **it;

            ImGuiWindowFlags windowFlags = osc::GetMinimalWindowFlags() & ~ImGuiWindowFlags_NoInputs;

            // if any layer above this one captures mouse inputs then disable this layer's inputs
            if (std::find_if(it+1, m_Layers.end(), [](auto const& layerPtr) -> bool { return layerPtr->getFlags() & ModelEditorViewerPanelLayerFlags_CapturesMouseInputs; }) != m_Layers.end())
            {
                windowFlags |= ImGuiWindowFlags_NoInputs;
            }

            // layers always have a background (although, it can be entirely invisible)
            windowFlags &= ~ImGuiWindowFlags_NoBackground;
            ImGui::SetNextWindowBgAlpha(layer.getBackgroundAlpha());

            // draw the layer in a child window, so that ImGui understands that hittests
            // should happen window-by-window (otherwise, you'll have problems with overlapping
            // buttons, widgets, etc.)
            ImGui::SetNextWindowPos(m_State.viewportRect.p1);
            if (ImGui::BeginChild(childWindowID, Dimensions(m_State.viewportRect), false, windowFlags))
            {
                layer.onDraw(m_Parameters, m_State);
                ImGui::EndChild();
            }

            ++childWindowID;
        }
    }

    void layersGarbageCollect()
    {
        osc::RemoveErase(m_Layers, [](auto const& layerPtr)
        {
            return layerPtr->shouldClose();
        });
    }

    void layersPopQueuedNewLayers()
    {
        m_State.flushLayerQueueTo(m_Layers);
    }

    ModelEditorViewerPanelParameters m_Parameters;
    ModelEditorViewerPanelState m_State{getName()};
    std::vector<std::unique_ptr<ModelEditorViewerPanelLayer>> m_Layers;
    bool m_IsFirstFrame = true;
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

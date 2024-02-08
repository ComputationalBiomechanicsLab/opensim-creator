#include "ModelEditorViewerPanel.hpp"

#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.hpp>
#include <OpenSimCreator/Graphics/CachedModelRenderer.hpp>
#include <OpenSimCreator/UI/Shared/BasicWidgets.hpp>
#include <OpenSimCreator/UI/Shared/ModelEditorViewerPanelLayer.hpp>
#include <OpenSimCreator/UI/Shared/ModelEditorViewerPanelParameters.hpp>
#include <OpenSimCreator/UI/Shared/ModelEditorViewerPanelRightClickEvent.hpp>
#include <OpenSimCreator/UI/Shared/ModelEditorViewerPanelState.hpp>
#include <OpenSimCreator/UI/Shared/ModelSelectionGizmo.hpp>
#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

#include <ImGuizmo.h>
#include <imgui.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Rect.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/Platform/Log.hpp>
#include <oscar/UI/IconCache.hpp>
#include <oscar/UI/ImGuiHelpers.hpp>
#include <oscar/UI/ImGuizmoHelpers.hpp>
#include <oscar/UI/Panels/StandardPanelImpl.hpp>
#include <oscar/UI/Widgets/GuiRuler.hpp>
#include <oscar/UI/Widgets/IconWithoutMenu.hpp>

#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace osc;

namespace
{
    std::string GetSettingsKeyPrefixForPanel(std::string_view panelName)
    {
        std::stringstream ss;
        ss << "panels/" << panelName << '/';
        return std::move(ss).str();
    }

    class RulerLayer final : public ModelEditorViewerPanelLayer {
    public:
        RulerLayer()
        {
            m_Ruler.startMeasuring();
        }

    private:
        ModelEditorViewerPanelLayerFlags implGetFlags() const final
        {
            return ModelEditorViewerPanelLayerFlags::CapturesMouseInputs;
        }

        bool implHandleMouseInputs(
            ModelEditorViewerPanelParameters&,
            ModelEditorViewerPanelState&) final
        {
            return true;  // always handles the mouse
        }

        void implOnDraw(
            ModelEditorViewerPanelParameters& params,
            ModelEditorViewerPanelState& state) final
        {
            m_Ruler.onDraw(
                params.getRenderParams().camera,
                state.viewportRect,
                state.maybeBaseLayerHittest
            );
        }

        bool implShouldClose() const final
        {
            return !m_Ruler.isMeasuring();
        }

        GuiRuler m_Ruler;
    };

    // model viewer layer that adds buttons for controling visualization
    // options and 3D manipulator gizmos
    class ButtonAndGizmoControlsLayer final : public ModelEditorViewerPanelLayer {
    public:
        ButtonAndGizmoControlsLayer(
            std::string_view panelName_,
            std::shared_ptr<UndoableModelStatePair> model_) :

            m_PanelName{panelName_},
            m_Gizmo{std::move(model_)}
        {
        }
    private:
        ModelEditorViewerPanelLayerFlags implGetFlags() const final
        {
            ModelEditorViewerPanelLayerFlags flags = ModelEditorViewerPanelLayerFlags::None;
            if (m_Gizmo.isUsing())
            {
                flags |= ModelEditorViewerPanelLayerFlags::CapturesMouseInputs;
            }
            return flags;
        }

        float implGetBackgroundAlpha() const final
        {
            return 0.0f;
        }

        bool implHandleMouseInputs(
            ModelEditorViewerPanelParameters&,
            ModelEditorViewerPanelState&) final
        {
            // care: ImGuizmo::isOver can return `true` even if it
            // isn't being drawn this frame
            return m_Gizmo.isUsing();
        }

        bool implHandleKeyboardInputs(
            ModelEditorViewerPanelParameters&,
            ModelEditorViewerPanelState&) final
        {
            return m_Gizmo.handleKeyboardInputs();
        }

        void implOnDraw(
            ModelEditorViewerPanelParameters& params,
            ModelEditorViewerPanelState& state) final
        {
            // draw generic overlays (i.e. the buttons for toggling things)
            auto renderParamsBefore = params.getRenderParams();

            bool const edited = DrawViewerImGuiOverlays(
                params.updRenderParams(),
                state.getDrawlist(),
                state.maybeSceneAABB,
                state.viewportRect,
                *m_IconCache,
                [this, &state]() { return drawExtraTopButtons(state); }
            );

            if (edited)
            {
                log_debug("%s edited", m_PanelName.c_str());

                auto const& renderParamsAfter = params.getRenderParams();

                SaveModelRendererParamsDifference(
                    renderParamsBefore,
                    renderParamsAfter,
                    GetSettingsKeyPrefixForPanel(m_PanelName),
                    App::upd().updConfig()
                );
            }

            // draw gizmo manipulators over the top
            m_Gizmo.onDraw(state.viewportRect, params.getRenderParams().camera);
        }

        bool implShouldClose() const final
        {
            return false;  // never closes
        }

        // draws extra top overlay buttons
        bool drawExtraTopButtons(
            ModelEditorViewerPanelState& state)
        {
            bool edited = false;

            IconWithoutMenu rulerButton
            {
                m_IconCache->getIcon("ruler"),
                "Ruler",
                "Roughly measure something in the scene",
            };
            if (rulerButton.onDraw())
            {
                state.pushLayer(std::make_unique<RulerLayer>());
                edited = true;
            }
            ImGui::SameLine();

            // draw translate/rotate/scale selector
            {
                ImGuizmo::OPERATION op = m_Gizmo.getOperation();
                if (DrawGizmoOpSelector(op, true, true, false))
                {
                    m_Gizmo.setOperation(op);
                    edited = true;
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
                    edited = true;
                }
            }

            return edited;
        }

        std::shared_ptr<IconCache> m_IconCache = App::singleton<IconCache>(
            App::resource_loader().withPrefix("icons/"),
            ImGui::GetTextLineHeight()/128.0f
        );
        std::string m_PanelName;
        ModelSelectionGizmo m_Gizmo;
    };

    // the "base" model viewer layer, which is the last layer to handle any input
    // etc. if no upper layer decides to handle it
    class BaseInteractionLayer final : public ModelEditorViewerPanelLayer {
    private:
        void implOnNewFrame() final
        {
            m_IsHandlingMouseInputs = false;

        }

        bool implHandleKeyboardInputs(
            ModelEditorViewerPanelParameters& params,
            ModelEditorViewerPanelState& state) final
        {
            return UpdatePolarCameraFromImGuiKeyboardInputs(
                params.updRenderParams().camera,
                state.viewportRect,
                state.maybeSceneAABB
            );
        }

        bool implHandleMouseInputs(
            ModelEditorViewerPanelParameters& params,
            ModelEditorViewerPanelState& state) final
        {
            m_IsHandlingMouseInputs = true;

            // try updating the camera (mouse panning, etc.)
            bool rv = UpdatePolarCameraFromImGuiMouseInputs(
                params.updRenderParams().camera,
                Dimensions(state.viewportRect)
            );

            if (IsDraggingWithAnyMouseButtonDown())
            {
                params.getModelSharedPtr()->setHovered(nullptr);
            }
            else if (state.maybeHoveredComponentAbsPath != GetAbsolutePathOrEmpty(params.getModelSharedPtr()->getHovered()))
            {
                // care: this code must check whether the hover != current hover
                // (even if null), because there might be multiple viewports open
                // (#582)
                params.getModelSharedPtr()->setHovered(
                    FindComponent(params.getModelSharedPtr()->getModel(), state.maybeHoveredComponentAbsPath)
                );
                rv = true;
            }

            // if left-clicked, update top-level model selection
            if (state.isLeftClickReleasedWithoutDragging)
            {
                params.getModelSharedPtr()->setSelected(
                    FindComponent(params.getModelSharedPtr()->getModel(), state.maybeHoveredComponentAbsPath)
                );
                rv = true;
            }

            return rv;
        }

        void implOnDraw(
            ModelEditorViewerPanelParameters& params,
            ModelEditorViewerPanelState& state) final
        {
            // hover, but not panning: show tooltip
            if (!state.maybeHoveredComponentAbsPath.toString().empty() &&
                m_IsHandlingMouseInputs &&
                !IsDraggingWithAnyMouseButtonDown())
            {
                if (OpenSim::Component const* c = FindComponent(params.getModelSharedPtr()->getModel(), state.maybeHoveredComponentAbsPath))
                {
                    DrawComponentHoverTooltip(*c);
                }
            }

            // right-click: open context menu
            if (m_IsHandlingMouseInputs &&
                state.isRightClickReleasedWithoutDragging)
            {
                // right-click: pump a right-click event
                ModelEditorViewerPanelRightClickEvent const e
                {
                    std::string{state.getPanelName()},
                    state.viewportRect,
                    state.maybeHoveredComponentAbsPath.toString(),
                    state.maybeBaseLayerHittest ? std::optional<Vec3>{state.maybeBaseLayerHittest->worldspaceLocation} : std::nullopt,
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

class osc::ModelEditorViewerPanel::Impl final : public StandardPanelImpl {
public:

    Impl(
        std::string_view panelName_,
        ModelEditorViewerPanelParameters parameters_) :

        StandardPanelImpl{panelName_},
        m_Parameters{std::move(parameters_)}
    {
        // update this panel's rendering/state parameters from the runtime
        // configuration (e.g. user edits)
        //
        // each panel has its own configuration set (`panels/viewer0,1,2, etc.`)
        UpdModelRendererParamsFrom(
            App::config(),
            GetSettingsKeyPrefixForPanel(panelName_),
            m_Parameters.updRenderParams()
        );
        pushLayer(std::make_unique<BaseInteractionLayer>());
        pushLayer(std::make_unique<ButtonAndGizmoControlsLayer>(panelName_, m_Parameters.getModelSharedPtr()));
    }

    ModelEditorViewerPanelLayer& pushLayer(std::unique_ptr<ModelEditorViewerPanelLayer> layer)
    {
        // care: do not push new layers directly into `m_Layers`, because `pushLayer` can be
        // called during iteration over `m_Layers` (e.g. during drawing)
        return m_State.pushLayer(std::move(layer));
    }

    void focusOn(Vec3 const& pos)
    {
        m_Parameters.updRenderParams().camera.focusPoint = -pos;
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
        // HACK: garbage-collect one frame later, because the layers
        // may have submitted textures to ImGui that are then invalid
        // because GCing destroyed them before they were rendered
        layersGarbageCollect();

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
        if (m_RenderIsHovered)
        {
            layersHandleMouseInputs();

            if (!ImGui::GetIO().WantCaptureKeyboard)
            {
                layersHandleKeyboardInputs();
            }
        }

        // render the 3D scene to a texture and present it via an ImGui::Image
        {
            RenderTexture& sceneTexture = m_State.updRenderer().onDraw(
                *m_Parameters.getModelSharedPtr(),
                m_Parameters.getRenderParams(),
                Dimensions(m_State.viewportRect),
                App::get().getCurrentAntiAliasingLevel()
            );
            DrawTextureAsImGuiImage(
                sceneTexture,
                Dimensions(m_State.viewportRect)
            );

            // care: hittesting is done here, rather than using ImGui::IsWindowHovered, because
            // we care about whether the _render_ is hovered, not any part of the window (which
            // may include things like the title bar, etc.
            //
            // screwing this up can result in unusual camera behavior, e.g. the camera may move when
            // dragging a visualizer panel around (#739 #93)

            // check if the window is conditionally hovered: this returns true if no other window is
            // overlapping the editor panel, _but_ it also returns true if the user is only hovering
            // the title bar of the window, rather than specifically the render
            bool const windowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);

            // check if the 3D render is hovered - ignore blocking and overlapping because the layer
            // stack might be screwing with this
            bool const renderHoveredIgnoringOverlap = ImGui::IsItemHovered(
                ImGuiHoveredFlags_AllowWhenBlockedByActiveItem |
                ImGuiHoveredFlags_AllowWhenOverlapped
            );

            m_RenderIsHovered = windowHovered && renderHoveredIgnoringOverlap;
        }

        // update state scene AABB
        m_State.maybeSceneAABB = m_State.getRenderer().getRootAABB();

        // if hovering in 2D, 3D-hittest the scene
        if (m_RenderIsHovered)
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
            m_State.maybeHoveredComponentAbsPath = m_State.maybeBaseLayerHittest->decorationID;
        }
        else
        {
            m_State.maybeHoveredComponentAbsPath = {};
        }

        layersDraw();
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
                (*it)->getFlags() & ModelEditorViewerPanelLayerFlags::CapturesMouseInputs)
            {
                return;
            }
        }
    }

    void layersDraw()
    {
        for (auto it = m_Layers.begin(); it != m_Layers.end(); ++it)
        {
            ModelEditorViewerPanelLayer& layer = **it;

            ImGuiWindowFlags windowFlags = GetMinimalWindowFlags() & ~ImGuiWindowFlags_NoInputs;

            // if any layer above this one captures mouse inputs then disable this layer's inputs
            if (std::find_if(it+1, m_Layers.end(), [](auto const& layerPtr) -> bool { return layerPtr->getFlags() & ModelEditorViewerPanelLayerFlags::CapturesMouseInputs; }) != m_Layers.end())
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
            std::string const childID = std::to_string(std::distance(it, m_Layers.end()));
            if (ImGui::BeginChild(childID.c_str(), Dimensions(m_State.viewportRect), false, windowFlags))
            {
                layer.onDraw(m_Parameters, m_State);
                ImGui::EndChild();
            }
        }
    }

    void layersGarbageCollect()
    {
        std::erase_if(m_Layers, [](auto const& layerPtr)
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
    bool m_RenderIsHovered = false;
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

ModelEditorViewerPanelLayer& osc::ModelEditorViewerPanel::pushLayer(std::unique_ptr<ModelEditorViewerPanelLayer> layer)
{
    return m_Impl->pushLayer(std::move(layer));
}

void osc::ModelEditorViewerPanel::focusOn(Vec3 const& pos)
{
    m_Impl->focusOn(pos);
}

CStringView osc::ModelEditorViewerPanel::implGetName() const
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

void osc::ModelEditorViewerPanel::implOnDraw()
{
    m_Impl->onDraw();
}

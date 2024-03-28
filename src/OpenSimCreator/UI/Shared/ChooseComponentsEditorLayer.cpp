#include "ChooseComponentsEditorLayer.h"

#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/Graphics/ModelRendererParams.h>
#include <OpenSimCreator/Graphics/OpenSimDecorationGenerator.h>
#include <OpenSimCreator/Graphics/OpenSimGraphicsHelpers.h>
#include <OpenSimCreator/Graphics/OverlayDecorationGenerator.h>
#include <OpenSimCreator/UI/Shared/BasicWidgets.h>
#include <OpenSimCreator/UI/Shared/ChooseComponentsEditorLayerParameters.h>
#include <OpenSimCreator/UI/Shared/ModelEditorViewerPanelParameters.h>
#include <OpenSimCreator/UI/Shared/ModelEditorViewerPanelState.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <IconsFontAwesome5.h>
#include <oscar/Graphics/Scene/SceneCache.h>
#include <oscar/Graphics/Scene/SceneDecoration.h>
#include <oscar/Graphics/Scene/SceneDecorationFlags.h>
#include <oscar/Graphics/Scene/SceneHelpers.h>
#include <oscar/Graphics/Scene/SceneRenderer.h>
#include <oscar/Maths/BVH.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Platform/App.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/Utils/CStringView.h>

#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace osc;

namespace
{
    // top-level shared state for the "choose components" layer
    struct ChooseComponentsEditorLayerSharedState final {

        explicit ChooseComponentsEditorLayerSharedState(
            std::shared_ptr<UndoableModelStatePair> model_,
            ChooseComponentsEditorLayerParameters parameters_) :

            model{std::move(model_)},
            popupParams{std::move(parameters_)}
        {
        }

        std::shared_ptr<SceneCache> meshCache = App::singleton<SceneCache>(App::resource_loader());
        std::shared_ptr<UndoableModelStatePair> model;
        ChooseComponentsEditorLayerParameters popupParams;
        ModelRendererParams renderParams;
        std::string hoveredComponent;
        std::unordered_set<std::string> alreadyChosenComponents;
        bool shouldClosePopup = false;
    };

    // grouping of scene (3D) decorations and an associated scene BVH
    struct BVHedDecorations final {
        void clear()
        {
            decorations.clear();
            bvh.clear();
        }

        std::vector<SceneDecoration> decorations;
        BVH bvh;
    };

    // generates scene decorations for the "choose components" layer
    void GenerateChooseComponentsDecorations(
        ChooseComponentsEditorLayerSharedState const& state,
        BVHedDecorations& out)
    {
        out.clear();

        auto const onModelDecoration = [&state, &out](OpenSim::Component const& component, SceneDecoration&& decoration)
        {
            // update flags based on path
            std::string const absPath = GetAbsolutePathString(component);
            if (state.popupParams.componentsBeingAssignedTo.contains(absPath))
            {
                decoration.flags |= SceneDecorationFlags::IsSelected;
            }
            if (state.alreadyChosenComponents.contains(absPath))
            {
                decoration.flags |= SceneDecorationFlags::IsSelected;
            }
            if (absPath == state.hoveredComponent)
            {
                decoration.flags |= SceneDecorationFlags::IsHovered;
            }

            if (state.popupParams.canChooseItem(component))
            {
                decoration.id = absPath;
            }
            else
            {
                decoration.color.a *= 0.2f;  // fade non-selectable objects
            }

            out.decorations.push_back(std::move(decoration));
        };

        GenerateModelDecorations(
            *state.meshCache,
            state.model->getModel(),
            state.model->getState(),
            state.renderParams.decorationOptions,
            state.model->getFixupScaleFactor(),
            onModelDecoration
        );

        update_scene_bvh(out.decorations, out.bvh);

        auto const onOverlayDecoration = [&](SceneDecoration&& decoration)
        {
            out.decorations.push_back(std::move(decoration));
        };

        GenerateOverlayDecorations(
            *state.meshCache,
            state.renderParams.overlayOptions,
            out.bvh,
            onOverlayDecoration
        );
    }
}

class osc::ChooseComponentsEditorLayer::Impl final {
public:
    Impl(
        std::shared_ptr<UndoableModelStatePair> model_,
        ChooseComponentsEditorLayerParameters parameters_) :

        m_State{std::move(model_), std::move(parameters_)},
        m_Renderer{*App::singleton<SceneCache>(App::resource_loader())}
    {}

    bool handleKeyboardInputs(
        ModelEditorViewerPanelParameters& params,
        ModelEditorViewerPanelState& state) const
    {
        return ui::UpdatePolarCameraFromKeyboardInputs(
            params.updRenderParams().camera,
            state.viewportRect,
            m_Decorations.bvh.getBounds()
        );
    }

    bool handleMouseInputs(
        ModelEditorViewerPanelParameters& params,
        ModelEditorViewerPanelState& state)
    {
        bool rv = ui::UpdatePolarCameraFromMouseInputs(
            params.updRenderParams().camera,
            dimensions_of(state.viewportRect)
        );

        if (ui::IsDraggingWithAnyMouseButtonDown())
        {
            m_State.hoveredComponent = {};
        }

        if (m_IsLeftClickReleasedWithoutDragging)
        {
            rv = tryToggleHover() || rv;
        }

        return rv;
    }

    void onDraw(
        ModelEditorViewerPanelParameters& panelParams,
        ModelEditorViewerPanelState& panelState)
    {
        bool const layerIsHovered = ui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

        // update this layer's state from provided state
        m_State.renderParams = panelParams.getRenderParams();
        m_IsLeftClickReleasedWithoutDragging = ui::IsMouseReleasedWithoutDragging(ImGuiMouseButton_Left);
        m_IsRightClickReleasedWithoutDragging = ui::IsMouseReleasedWithoutDragging(ImGuiMouseButton_Right);
        if (ui::IsKeyReleased(ImGuiKey_Escape))
        {
            m_State.shouldClosePopup = true;
        }

        // generate decorations + rendering params
        GenerateChooseComponentsDecorations(m_State, m_Decorations);
        SceneRendererParams const rendererParameters = CalcSceneRendererParams(
            m_State.renderParams,
            dimensions_of(panelState.viewportRect),
            App::get().getCurrentAntiAliasingLevel(),
            m_State.model->getFixupScaleFactor()
        );

        // render to a texture (no caching)
        m_Renderer.render(m_Decorations.decorations, rendererParameters);

        // blit texture as ImGui image
        ui::Image(
            m_Renderer.upd_render_texture(),
            dimensions_of(panelState.viewportRect)
        );

        // do hovertest
        if (layerIsHovered)
        {
            std::optional<SceneCollision> const collision = GetClosestCollision(
                m_Decorations.bvh,
                *m_State.meshCache,
                m_Decorations.decorations,
                m_State.renderParams.camera,
                ui::GetMousePos(),
                panelState.viewportRect
            );
            if (collision)
            {
                m_State.hoveredComponent = collision->decoration_id;
            }
            else
            {
                m_State.hoveredComponent = {};
            }
        }

        // show tooltip
        if (OpenSim::Component const* c = FindComponent(m_State.model->getModel(), m_State.hoveredComponent))
        {
            DrawComponentHoverTooltip(*c);
        }

        // show header
        ui::SetCursorScreenPos(panelState.viewportRect.p1 + Vec2{10.0f, 10.0f});
        ui::Text("%s (ESC to cancel)", m_State.popupParams.popupHeaderText.c_str());

        // handle completion state (i.e. user selected enough components)
        if (m_State.alreadyChosenComponents.size() == m_State.popupParams.numComponentsUserMustChoose)
        {
            m_State.popupParams.onUserFinishedChoosing(m_State.alreadyChosenComponents);
            m_State.shouldClosePopup = true;
        }

        // draw cancellation button
        {
            ui::PushStyleVar(ImGuiStyleVar_FramePadding, {10.0f, 10.0f});

            constexpr CStringView cancellationButtonText = ICON_FA_ARROW_LEFT " Cancel (ESC)";
            Vec2 const margin = {25.0f, 25.0f};
            Vec2 const buttonDims = ui::CalcButtonSize(cancellationButtonText);
            Vec2 const buttonTopLeft = panelState.viewportRect.p2 - (buttonDims + margin);
            ui::SetCursorScreenPos(buttonTopLeft);
            if (ui::Button(cancellationButtonText))
            {
                m_State.shouldClosePopup = true;
            }

            ui::PopStyleVar();
        }
    }

    float getBackgroundAlpha() const
    {
        return 1.0f;
    }

    bool shouldClose() const
    {
        return m_State.shouldClosePopup;
    }

    bool tryToggleHover()
    {
        std::string const& absPath = m_State.hoveredComponent;
        OpenSim::Component const* component = FindComponent(m_State.model->getModel(), absPath);

        if (!component)
        {
            return false;  // nothing hovered
        }
        else if (m_State.popupParams.componentsBeingAssignedTo.contains(absPath))
        {
            return false;  // cannot be selected
        }
        else if (auto it = m_State.alreadyChosenComponents.find(absPath); it != m_State.alreadyChosenComponents.end())
        {
            m_State.alreadyChosenComponents.erase(it);
            return true;   // de-selected
        }
        else if (
            m_State.alreadyChosenComponents.size() < m_State.popupParams.numComponentsUserMustChoose &&
            m_State.popupParams.canChooseItem(*component))
        {
            m_State.alreadyChosenComponents.insert(absPath);
            return true;   // selected
        }
        else
        {
            return false;  // don't know how to handle
        }
    }

    ChooseComponentsEditorLayerSharedState m_State;
    BVHedDecorations m_Decorations;
    SceneRenderer m_Renderer;
    bool m_IsLeftClickReleasedWithoutDragging = false;
    bool m_IsRightClickReleasedWithoutDragging = false;
};

osc::ChooseComponentsEditorLayer::ChooseComponentsEditorLayer(
    std::shared_ptr<UndoableModelStatePair> model_,
    ChooseComponentsEditorLayerParameters parameters_) :

    m_Impl{std::make_unique<Impl>(std::move(model_), std::move(parameters_))}
{
}
osc::ChooseComponentsEditorLayer::ChooseComponentsEditorLayer(ChooseComponentsEditorLayer&&) noexcept = default;
osc::ChooseComponentsEditorLayer& osc::ChooseComponentsEditorLayer::operator=(ChooseComponentsEditorLayer&&) noexcept = default;
osc::ChooseComponentsEditorLayer::~ChooseComponentsEditorLayer() noexcept = default;

bool osc::ChooseComponentsEditorLayer::implHandleKeyboardInputs(
    ModelEditorViewerPanelParameters& params,
    ModelEditorViewerPanelState& state)
{
    return m_Impl->handleKeyboardInputs(params, state);
}

bool osc::ChooseComponentsEditorLayer::implHandleMouseInputs(
    ModelEditorViewerPanelParameters& params,
    ModelEditorViewerPanelState& state)
{
    return m_Impl->handleMouseInputs(params, state);
}

void osc::ChooseComponentsEditorLayer::implOnDraw(
    ModelEditorViewerPanelParameters& params,
    ModelEditorViewerPanelState& state)
{
    m_Impl->onDraw(params, state);
}

float osc::ChooseComponentsEditorLayer::implGetBackgroundAlpha() const
{
    return m_Impl->getBackgroundAlpha();
}

bool osc::ChooseComponentsEditorLayer::implShouldClose() const
{
    return m_Impl->shouldClose();
}

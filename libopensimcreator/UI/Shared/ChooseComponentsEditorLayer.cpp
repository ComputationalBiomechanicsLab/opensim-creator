#include "ChooseComponentsEditorLayer.h"

#include <libopensimcreator/Documents/Model/IModelStatePair.h>
#include <libopensimcreator/Graphics/ModelRendererParams.h>
#include <libopensimcreator/Graphics/OpenSimDecorationGenerator.h>
#include <libopensimcreator/Graphics/OpenSimGraphicsHelpers.h>
#include <libopensimcreator/Graphics/OverlayDecorationGenerator.h>
#include <libopensimcreator/Platform/IconCodepoints.h>
#include <libopensimcreator/UI/Shared/BasicWidgets.h>
#include <libopensimcreator/UI/Shared/ChooseComponentsEditorLayerParameters.h>
#include <libopensimcreator/UI/Shared/ModelViewerPanelParameters.h>
#include <libopensimcreator/UI/Shared/ModelViewerPanelState.h>
#include <libopensimcreator/Utils/OpenSimHelpers.h>

#include <liboscar/Graphics/Scene/SceneCache.h>
#include <liboscar/Graphics/Scene/SceneDecoration.h>
#include <liboscar/Graphics/Scene/SceneDecorationFlags.h>
#include <liboscar/Graphics/Scene/SceneHelpers.h>
#include <liboscar/Graphics/Scene/SceneRenderer.h>
#include <liboscar/Maths/BVH.h>
#include <liboscar/Maths/MathHelpers.h>
#include <liboscar/Maths/RectFunctions.h>
#include <liboscar/Maths/Vector2.h>
#include <liboscar/Platform/App.h>
#include <liboscar/Platform/AppSettings.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/Utils/CStringView.h>
#include <liboscar/Utils/StringName.h>

#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

using namespace osc;

namespace
{
    // top-level shared state for the "choose components" layer
    struct ChooseComponentsEditorLayerSharedState final {

        explicit ChooseComponentsEditorLayerSharedState(
            std::shared_ptr<IModelStatePair> model_,
            ChooseComponentsEditorLayerParameters parameters_) :

            model{std::move(model_)},
            popupParams{std::move(parameters_)}
        {
        }

        std::shared_ptr<SceneCache> meshCache = App::singleton<SceneCache>(App::resource_loader());
        std::shared_ptr<IModelStatePair> model;
        ChooseComponentsEditorLayerParameters popupParams;
        ModelRendererParams renderParams;
        StringName hoveredComponent;
        std::unordered_set<StringName> alreadyChosenComponents;
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
        const ChooseComponentsEditorLayerSharedState& state,
        BVHedDecorations& out)
    {
        out.clear();

        const auto onModelDecoration = [&state, &out](const OpenSim::Component& component, SceneDecoration&& decoration)
        {
            // update flags based on path
            const StringName absPath = GetAbsolutePathStringName(component);
            if (state.popupParams.componentsBeingAssignedTo.contains(absPath) or
                state.alreadyChosenComponents.contains(absPath)) {

                decoration.flags |= SceneDecorationFlag::RimHighlight0;
            }
            if (absPath == state.hoveredComponent) {
                decoration.flags |= SceneDecorationFlag::RimHighlight1;
            }

            if (state.popupParams.canChooseItem(component)) {
                decoration.id = absPath;
            }
            else if (std::holds_alternative<Color>(decoration.shading)) {
                std::get<Color>(decoration.shading).a *= 0.2f;  // fade non-selectable objects
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

        const auto onOverlayDecoration = [&](SceneDecoration&& decoration)
        {
            out.decorations.push_back(std::move(decoration));
        };

        GenerateOverlayDecorations(
            *state.meshCache,
            state.renderParams.overlayOptions,
            out.bvh,
            state.model->getFixupScaleFactor(),
            onOverlayDecoration
        );
    }
}

class osc::ChooseComponentsEditorLayer::Impl final {
public:
    Impl(
        std::shared_ptr<IModelStatePair> model_,
        ChooseComponentsEditorLayerParameters parameters_) :

        m_State{std::move(model_), std::move(parameters_)},
        m_Renderer{*App::singleton<SceneCache>(App::resource_loader())}
    {}

    bool handleKeyboardInputs(
        ModelViewerPanelParameters& params,
        ModelViewerPanelState& state) const
    {
        return ui::update_polar_camera_from_keyboard_inputs(
            params.updRenderParams().camera,
            state.viewportUiRect,
            m_Decorations.bvh.bounds()
        );
    }

    bool handleMouseInputs(
        ModelViewerPanelParameters& params,
        ModelViewerPanelState& state)
    {
        bool rv = ui::update_polar_camera_from_mouse_inputs(
            params.updRenderParams().camera,
            state.viewportUiRect.dimensions()
        );

        if (ui::is_mouse_dragging_with_any_button_down())
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
        ModelViewerPanelParameters& panelParams,
        ModelViewerPanelState& panelState)
    {
        const bool layerIsHovered = ui::is_panel_hovered(ui::HoveredFlag::RootAndChildPanels);

        // update this layer's state from provided state
        m_State.renderParams = panelParams.getRenderParams();
        m_IsLeftClickReleasedWithoutDragging = ui::is_mouse_released_without_dragging(ui::MouseButton::Left);
        m_IsRightClickReleasedWithoutDragging = ui::is_mouse_released_without_dragging(ui::MouseButton::Right);
        if (ui::is_key_released(Key::Escape))
        {
            m_State.shouldClosePopup = true;
        }

        // generate decorations + rendering params
        GenerateChooseComponentsDecorations(m_State, m_Decorations);
        const SceneRendererParams rendererParameters = CalcSceneRendererParams(
            m_State.renderParams,
            panelState.viewportUiRect.dimensions(),
            App::settings().get_value<float>("graphics/render_scale", 1.0f) * App::get().main_window_device_pixel_ratio(),
            App::get().anti_aliasing_level(),
            m_State.model->getFixupScaleFactor()
        );

        // render to a texture (no caching)
        m_Renderer.render(m_Decorations.decorations, rendererParameters);

        // blit texture as ImGui image
        ui::draw_image(
            m_Renderer.upd_render_texture(),
            panelState.viewportUiRect.dimensions()
        );

        // do hovertest
        if (layerIsHovered)
        {
            const std::optional<SceneCollision> collision = GetClosestCollision(
                m_Decorations.bvh,
                *m_State.meshCache,
                m_Decorations.decorations,
                m_State.renderParams.camera,
                ui::get_mouse_ui_position(),
                panelState.viewportUiRect
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
        if (const OpenSim::Component* c = FindComponent(m_State.model->getModel(), m_State.hoveredComponent))
        {
            DrawComponentHoverTooltip(*c);
        }

        // show header
        ui::set_cursor_ui_position(panelState.viewportUiRect.ypd_top_left() + Vector2{10.0f, 10.0f});
        ui::draw_text("%s (ESC to cancel)", m_State.popupParams.popupHeaderText.c_str());

        // handle completion state (i.e. user selected enough components)
        if (m_State.alreadyChosenComponents.size() == m_State.popupParams.numComponentsUserMustChoose)
        {
            m_State.popupParams.onUserFinishedChoosing(m_State.alreadyChosenComponents);
            m_State.shouldClosePopup = true;
        }

        // draw cancellation button
        {
            ui::push_style_var(ui::StyleVar::FramePadding, {10.0f, 10.0f});

            constexpr CStringView cancellationButtonText = OSC_ICON_ARROW_LEFT " Cancel (ESC)";
            const Vector2 margin = {25.0f, 25.0f};
            const Vector2 buttonDims = ui::calc_button_size(cancellationButtonText);
            const Vector2 buttonTopLeft = panelState.viewportUiRect.ypd_bottom_right() - (buttonDims + margin);
            ui::set_cursor_ui_position(buttonTopLeft);
            if (ui::draw_button(cancellationButtonText))
            {
                m_State.shouldClosePopup = true;
            }

            ui::pop_style_var();
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
        const auto& absPath = m_State.hoveredComponent;
        const OpenSim::Component* component = FindComponent(m_State.model->getModel(), absPath);

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
    std::shared_ptr<IModelStatePair> model_,
    ChooseComponentsEditorLayerParameters parameters_) :

    m_Impl{std::make_unique<Impl>(std::move(model_), std::move(parameters_))}
{
}
osc::ChooseComponentsEditorLayer::ChooseComponentsEditorLayer(ChooseComponentsEditorLayer&&) noexcept = default;
osc::ChooseComponentsEditorLayer& osc::ChooseComponentsEditorLayer::operator=(ChooseComponentsEditorLayer&&) noexcept = default;
osc::ChooseComponentsEditorLayer::~ChooseComponentsEditorLayer() noexcept = default;

bool osc::ChooseComponentsEditorLayer::implHandleKeyboardInputs(
    ModelViewerPanelParameters& params,
    ModelViewerPanelState& state)
{
    return m_Impl->handleKeyboardInputs(params, state);
}

bool osc::ChooseComponentsEditorLayer::implHandleMouseInputs(
    ModelViewerPanelParameters& params,
    ModelViewerPanelState& state)
{
    return m_Impl->handleMouseInputs(params, state);
}

void osc::ChooseComponentsEditorLayer::implOnDraw(
    ModelViewerPanelParameters& params,
    ModelViewerPanelState& state)
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

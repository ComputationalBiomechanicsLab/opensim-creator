#include "model_viewer_panel.h"

#include <libopensimcreator/platform/msmicons.h>
#include <libopensimcreator/ui/shared/basic_widgets.h>
#include <libopensimcreator/ui/shared/model_selection_gizmo.h>
#include <libopensimcreator/ui/shared/model_viewer_panel_layer.h>
#include <libopensimcreator/ui/shared/model_viewer_panel_parameters.h>
#include <libopensimcreator/ui/shared/model_viewer_panel_right_click_event.h>
#include <libopensimcreator/ui/shared/model_viewer_panel_state.h>

#include <libopynsim/documents/model/model_state_pair.h>
#include <libopynsim/documents/model/model_state_pair_info.h>
#include <libopynsim/graphics/cached_model_renderer.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <libopynsim/utilities/simbody_x_oscar.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/rect.h>
#include <liboscar/maths/rect_functions.h>
#include <liboscar/maths/vector3.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/app_settings.h>
#include <liboscar/platform/log.h>
#include <liboscar/ui/icon_cache.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/panels/panel_private.h>
#include <liboscar/ui/widgets/gui_ruler.h>
#include <liboscar/ui/widgets/icon_without_menu.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>

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

    class RulerLayer final : public ModelViewerPanelLayer {
    public:
        RulerLayer()
        {
            m_Ruler.start_measuring();
        }

    private:
        ModelViewerPanelLayerFlags implGetFlags() const final
        {
            return ModelViewerPanelLayerFlags::CapturesMouseInputs;
        }

        bool implHandleMouseInputs(
            ModelViewerPanelParameters&,
            ModelViewerPanelState&) final
        {
            return true;  // always handles the mouse
        }

        void implOnDraw(
            ModelViewerPanelParameters& params,
            ModelViewerPanelState& state) final
        {
            m_Ruler.on_draw(
                params.getRenderParams().camera,
                state.viewportUiRect,
                state.maybeBaseLayerHittest
            );
        }

        bool implShouldClose() const final
        {
            return !m_Ruler.is_measuring();
        }

        GuiRuler m_Ruler;
    };

    // model viewer layer that adds buttons for controling visualization
    // options and 3D manipulator gizmos
    class ButtonAndGizmoControlsLayer final : public ModelViewerPanelLayer {
    public:
        ButtonAndGizmoControlsLayer(
            std::string_view panelName_,
            std::shared_ptr<opyn::ModelStatePair> model_) :

            panel_name_{panelName_},
            m_Gizmo{std::move(model_)}
        {}
    private:
        ModelViewerPanelLayerFlags implGetFlags() const final
        {
            ModelViewerPanelLayerFlags flags = ModelViewerPanelLayerFlags::None;
            if (m_Gizmo.isUsing())
            {
                flags |= ModelViewerPanelLayerFlags::CapturesMouseInputs;
            }
            return flags;
        }

        float implGetBackgroundAlpha() const final
        {
            return 0.0f;
        }

        bool implHandleMouseInputs(
            ModelViewerPanelParameters&,
            ModelViewerPanelState&) final
        {
            // care: `isUsing` can return `true` even if it isn't being drawn this frame
            return m_Gizmo.isUsing();
        }

        bool implHandleKeyboardInputs(
            ModelViewerPanelParameters&,
            ModelViewerPanelState&) final
        {
            return m_Gizmo.handleKeyboardInputs();
        }

        void implOnDraw(
            ModelViewerPanelParameters& params,
            ModelViewerPanelState& state) final
        {
            // draw generic overlays (i.e. the buttons for toggling things)
            auto renderParamsBefore = params.getRenderParams();

            const bool edited = DrawViewerImGuiOverlays(
                params.updRenderParams(),
                state.getDrawlist(),
                state.maybeSceneVisibleAABB,
                state.viewportUiRect,
                *m_IconCache,
                [this, &state]() { return drawExtraTopButtons(state); }
            );

            if (edited)
            {
                log_debug("%s edited", panel_name_.c_str());

                const auto& renderParamsAfter = params.getRenderParams();

                SaveModelRendererParamsDifference(
                    renderParamsBefore,
                    renderParamsAfter,
                    GetSettingsKeyPrefixForPanel(panel_name_),
                    App::upd().upd_settings()
                );
            }

            // draw gizmo manipulators over the top
            m_Gizmo.onDraw(state.viewportUiRect, params.getRenderParams().camera);
        }

        bool implShouldClose() const final
        {
            return false;  // never closes
        }

        // draws extra top overlay buttons
        bool drawExtraTopButtons(
            ModelViewerPanelState& state)
        {
            bool edited = false;

            IconWithoutMenu rulerButton
            {
                m_IconCache->find_or_throw("ruler"),
                "Ruler",
                "Roughly measure something in the scene",
            };
            if (rulerButton.on_draw())
            {
                state.pushLayer(std::make_unique<RulerLayer>());
                edited = true;
            }

            ui::same_line();
            ui::draw_vertical_separator();
            ui::same_line();

            // draw translate/rotate/scale selector
            if (ui::draw_gizmo_operation_selector(m_Gizmo, true, true, false, MSMICONS_ARROWS_ALT, MSMICONS_REDO, MSMICONS_EXPAND_ARROWS_ALT)) {
                edited = true;
            }

            ui::push_style_var(ui::StyleVar::ItemSpacing, {0.0f, 0.0f});
            ui::same_line();
            ui::pop_style_var();

            // draw global/world selector
            if (ui::draw_gizmo_mode_selector(m_Gizmo)) {
                edited = true;
            }

            return edited;
        }

        std::shared_ptr<IconCache> m_IconCache = App::singleton<IconCache>(
            App::resource_loader().with_prefix("OpenSimCreator/icons/"),
            ui::get_font_base_size()/128.0f,
            App::get().highest_device_pixel_ratio()
        );
        std::string panel_name_;
        ModelSelectionGizmo m_Gizmo;
    };

    // `ModelViewerPanelLayer` that overlays non-interactive 2D annotations
    // over the 3D render.
    class InformationalOverlaysLayer final : public ModelViewerPanelLayer {
    public:
        explicit InformationalOverlaysLayer() = default;
    private:
        ModelViewerPanelLayerFlags implGetFlags() const final
        {
            return ModelViewerPanelLayerFlags::None;
        }

        void implOnDraw(ModelViewerPanelParameters& params, ModelViewerPanelState& state) final
        {
            drawCoordinateLineOverlayOrResetCache(params, state);
        }

        void drawCoordinateLineOverlayOrResetCache(ModelViewerPanelParameters& params, ModelViewerPanelState& state)
        {
            // Update cached `OpenSim::Joint` transform samples with respect to the
            // selected `OpenSim::Coordinate`, if applicable.
            auto& cache = m_CachedCoordinateOverlayState;
            if (opyn::ModelStatePairInfo currentModelStatePair{*params.getModelSharedPtr()};
                currentModelStatePair != m_PreviousModelStatePair) {

                // Cache key has changed: clear/reset cached stuff.
                cache.clear();

                if (const auto* coordinate = params.getModelSharedPtr()->getSelectedAs<OpenSim::Coordinate>();
                    (coordinate != nullptr) and not coordinate->getLocked(params.getModelSharedPtr()->getState())) {

                    // If the caller has an `OpenSim::Coordinate` selected, and it isn't
                    // locked, sample `[min, max]` to figure out how the joint transform
                    // changes with respect to the coordinate.
                    try {
                        const auto& associatedJoint = coordinate->getJoint();
                        const auto& associatedChildFrame = associatedJoint.getChildFrame();
                        const ClosedInterval<double> coordinateRange{coordinate->getRangeMin(), coordinate->getRangeMax()};
                        const double samplerStepSize = coordinateRange.step_size(c_NumCoordinateSamplePoints);

                        // Save current joint transform (used for showing user current state-of-play).
                        SimTK::State samplingState{params.getModelSharedPtr()->getState()};
                        params.getModelSharedPtr()->getModel().realizePosition(samplingState);
                        cache.currentTransform = associatedChildFrame.getTransformInGround(samplingState);

                        // Sample along the coordinate, caching transforms.
                        cache.sampledTransforms.reserve(c_NumCoordinateSamplePoints);
                        for (size_t step = 0; step < c_NumCoordinateSamplePoints; ++step) {
                            const double sampledCoordinateValue = coordinateRange.lower + static_cast<double>(step)*samplerStepSize;
                            coordinate->setValue(samplingState, sampledCoordinateValue, false);
                            params.getModelSharedPtr()->getModel().realizePosition(samplingState);
                            cache.sampledTransforms.push_back(associatedChildFrame.getTransformInGround(samplingState));
                        }
                    }
                    catch (const std::exception& ex) {
                        // There was a problem sampling the points.
                        //
                        // Because this is a visual overlay, we recover by just not showing the overlay.
                        cache.clear();
                        log_warn("ModelViewerPanel: error sampling selected coordinate %s: %s",
                            coordinate->getName().c_str(),
                            ex.what()
                        );
                    }
                }
                m_PreviousModelStatePair = std::move(currentModelStatePair);  // Update cache key
            }

            // If the user (still) has an `OpenSim::Coordinate` selected, and the cache is
            // populated with enough data, draw the overlay.
            if (const auto* coordinate = params.getModelSharedPtr()->getSelectedAs<OpenSim::Coordinate>();
                coordinate != nullptr and cache.sampledTransforms.size() >= 2) {

                // Represents points projected from a transform into ui space.
                struct ProjectedPoints {
                    ProjectedPoints() = default;

                    explicit ProjectedPoints(
                        const PolarPerspectiveCamera& camera,
                        const Rect& viewportUiRect,
                        const SimTK::Transform& t)
                    {
                        const float viewportFillPercentage = c_FrameLegProjectionInScreenSpace / viewportUiRect.height();
                        const float scale = viewportFillPercentage * camera.frustum_height_at_depth(camera.radius);

                        const auto worldOrigin = to<Vector3>(t.shiftFrameStationToBase(SimTK::Vec3(0.0,   0.0,   0.0  )));
                        const auto worldX      = to<Vector3>(t.shiftFrameStationToBase(SimTK::Vec3(scale, 0.0,   0.0  )));
                        const auto worldY      = to<Vector3>(t.shiftFrameStationToBase(SimTK::Vec3(0.0,   scale, 0.0  )));
                        const auto worldZ      = to<Vector3>(t.shiftFrameStationToBase(SimTK::Vec3(0.0,   0.0,   scale)));

                        origin = camera.project_onto_viewport(worldOrigin, viewportUiRect);
                        x      = camera.project_onto_viewport(worldX, viewportUiRect);
                        y      = camera.project_onto_viewport(worldY, viewportUiRect);
                        z      = camera.project_onto_viewport(worldZ, viewportUiRect);
                    }

                    Vector2 origin, x, y, z;
                };

                auto dl = ui::get_panel_draw_list();
                const auto& viewportUiRect = state.viewportUiRect;
                const auto& camera = params.getRenderParams().camera;

                // Draw lines along the sample points, so users can see
                // the "rail" of the coordinate.
                {
                    ProjectedPoints previous{camera, viewportUiRect, cache.sampledTransforms[0]};
                    for (size_t i = 1; i < cache.sampledTransforms.size(); ++i) {
                        ProjectedPoints current{camera, viewportUiRect, cache.sampledTransforms[i]};

                        dl.add_line(previous.x, current.x, Color::red()  .with_alpha(c_CoordinateAxisAlpha), c_OverlayThickness);
                        dl.add_line(previous.y, current.y, Color::green().with_alpha(c_CoordinateAxisAlpha), c_OverlayThickness);
                        dl.add_line(previous.z, current.z, Color::blue() .with_alpha(c_CoordinateAxisAlpha), c_OverlayThickness);

                        previous = current;
                    }
                }

                // If the `OpenSim::Coordinate` is clamped, put an endcap on the rail, so that
                // users can see that a coordinate must stop at the ends.
                if (coordinate->getClamped(params.getModelSharedPtr()->getState())) {

                    const auto drawCaps = [](ui::DrawListView& dl, const ProjectedPoints& a, const ProjectedPoints& b)
                    {
                        const auto drawCap = [](ui::DrawListView& dl, const Vector2& p1, const Vector2& p2, const Color& color, float offset)
                        {
                            const Vector2 delta = p2 - p1;
                            const float deltaLength = length(delta);
                            if (deltaLength < 0.0001f) {
                                return;  // Don't draw a cap if the last two points are basically on top of each other
                            }
                            const Vector2 lineDirection = delta / deltaLength;
                            const Vector2 endpointWithOffset = p2 + offset*lineDirection;
                            const Vector2 endcapNorm{-lineDirection.y(), lineDirection.x()};
                            const Vector2 endcapStart = endpointWithOffset - (2.0f*c_OverlayThickness*endcapNorm);
                            const Vector2 endcapEnd = endpointWithOffset + (2.0f*c_OverlayThickness*endcapNorm);

                            dl.add_line(endcapStart, endcapEnd, color, c_OverlayThickness);
                        };
                        drawCap(dl, b.origin, a.origin, Color::white().with_alpha(c_CoordinateAxisAlpha), 0.5f*c_OverlayThickness);
                        drawCap(dl, b.x,      a.x,      Color::red()  .with_alpha(c_CoordinateAxisAlpha), 0.5f*c_OverlayThickness);
                        drawCap(dl, b.y,      a.y,      Color::green().with_alpha(c_CoordinateAxisAlpha), 0.5f*c_OverlayThickness);
                        drawCap(dl, b.z,      a.z,      Color::blue() .with_alpha(c_CoordinateAxisAlpha), 0.5f*c_OverlayThickness);
                    };

                    // Min caps
                    drawCaps(
                        dl,
                        ProjectedPoints{camera, viewportUiRect, cache.sampledTransforms[0]},
                        ProjectedPoints{camera, viewportUiRect, cache.sampledTransforms[1]}
                    );

                    // Max caps
                    drawCaps(
                        dl,
                        ProjectedPoints(camera, viewportUiRect, cache.sampledTransforms.back()),
                        ProjectedPoints(camera, viewportUiRect, *(cache.sampledTransforms.rbegin()+1))
                    );
                }

                // Draw a frame-like core representing the coordinate's current state.
                {
                    const ProjectedPoints pp{camera, viewportUiRect, cache.currentTransform};

                    // Legs
                    dl.add_line(pp.origin, pp.x, Color::red(),   c_OverlayThickness);
                    dl.add_line(pp.origin, pp.y, Color::green(), c_OverlayThickness);
                    dl.add_line(pp.origin, pp.z, Color::blue(),  c_OverlayThickness);

                    // Circles
                    dl.add_circle_filled({.origin = pp.x,      .radius = c_CoreRadius}, Color::red());
                    dl.add_circle_filled({.origin = pp.y,      .radius = c_CoreRadius}, Color::green());
                    dl.add_circle_filled({.origin = pp.z,      .radius = c_CoreRadius}, Color::blue());
                    dl.add_circle_filled({.origin = pp.origin, .radius = c_CoreRadius}, Color::white());
                }
            }
        }

        opyn::ModelStatePairInfo m_PreviousModelStatePair;
        struct CachedCoordinateOverlayState {
            void clear() { sampledTransforms.clear(); }

            SimTK::Transform currentTransform;
            std::vector<SimTK::Transform> sampledTransforms;
        };
        CachedCoordinateOverlayState m_CachedCoordinateOverlayState;
        static constexpr size_t c_NumCoordinateSamplePoints = 100;
        static constexpr float c_FrameLegProjectionInScreenSpace = 128.0f;
        static constexpr float c_OverlayThickness = 5.0f;
        static constexpr float c_CoreRadius = 1.25f*c_OverlayThickness;
        static constexpr float c_CoordinateAxisAlpha = 0.45f;
    };

    // the "base" model viewer layer, which is the last layer to handle any input
    // etc. if no upper layer decides to handle it
    class BaseInteractionLayer final : public ModelViewerPanelLayer {
    private:
        void implOnNewFrame() final
        {
            m_IsHandlingMouseInputs = false;

        }

        bool implHandleKeyboardInputs(
            ModelViewerPanelParameters& params,
            ModelViewerPanelState& state) final
        {
            return ui::update_polar_camera_from_keyboard_inputs(
                params.updRenderParams().camera,
                state.viewportUiRect,
                state.maybeSceneVisibleAABB
            );
        }

        bool implHandleMouseInputs(
            ModelViewerPanelParameters& params,
            ModelViewerPanelState& state) final
        {
            m_IsHandlingMouseInputs = true;

            // try updating the camera (mouse panning, etc.)
            bool rv = ui::update_polar_camera_from_mouse_inputs(
                params.updRenderParams().camera,
                state.viewportUiRect.dimensions()
            );

            if (ui::is_mouse_dragging_with_any_button_down())
            {
                params.getModelSharedPtr()->setHovered(nullptr);
            }
            else if (state.maybeHoveredComponentAbsPath != opyn::GetAbsolutePathOrEmpty(params.getModelSharedPtr()->getHovered()))
            {
                // care: this code must check whether the hover != current hover
                // (even if null), because there might be multiple viewports open
                // (#582)
                params.getModelSharedPtr()->setHovered(
                    opyn::FindComponent(params.getModelSharedPtr()->getModel(), state.maybeHoveredComponentAbsPath)
                );
                rv = true;
            }

            // if left-clicked, update top-level model selection
            if (state.isLeftClickReleasedWithoutDragging)
            {
                params.getModelSharedPtr()->setSelected(
                    opyn::FindComponent(params.getModelSharedPtr()->getModel(), state.maybeHoveredComponentAbsPath)
                );
                rv = true;
            }

            return rv;
        }

        void implOnDraw(
            ModelViewerPanelParameters& params,
            ModelViewerPanelState& state) final
        {
            // hover, but not panning: show tooltip
            if (!state.maybeHoveredComponentAbsPath.toString().empty() &&
                m_IsHandlingMouseInputs &&
                !ui::is_mouse_dragging_with_any_button_down())
            {
                if (const OpenSim::Component* c = opyn::FindComponent(params.getModelSharedPtr()->getModel(), state.maybeHoveredComponentAbsPath))
                {
                    DrawComponentHoverTooltip(*c);
                }
            }

            // right-click: open context menu
            if (m_IsHandlingMouseInputs &&
                state.isRightClickReleasedWithoutDragging)
            {
                // right-click: pump a right-click event
                const ModelViewerPanelRightClickEvent e
                {
                    std::string{state.getPanelName()},
                    state.viewportUiRect,
                    state.maybeHoveredComponentAbsPath.toString(),
                    state.maybeBaseLayerHittest ? std::optional<Vector3>{state.maybeBaseLayerHittest->world_position} : std::nullopt,
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

class osc::ModelViewerPanel::Impl final : public PanelPrivate {
public:

    explicit Impl(
        ModelViewerPanel& owner_,
        Widget* parent_,
        std::string_view panelName_,
        ModelViewerPanelParameters parameters_,
        ModelViewerPanelFlags flags_) :

        PanelPrivate{owner_, parent_, panelName_},
        m_Parameters{std::move(parameters_)},
        m_State{name(), flags_}
    {
        // update this panel's rendering/state parameters from the runtime
        // configuration (e.g. user edits)
        //
        // each panel has its own configuration set (`panels/viewer0,1,2, etc.`)
        UpdModelRendererParamsFrom(
            App::settings(),
            GetSettingsKeyPrefixForPanel(panelName_),
            m_Parameters.updRenderParams()
        );
        pushLayer(std::make_unique<BaseInteractionLayer>());
        pushLayer(std::make_unique<InformationalOverlaysLayer>());
        pushLayer(std::make_unique<ButtonAndGizmoControlsLayer>(panelName_, m_Parameters.getModelSharedPtr()));
    }

    bool isMousedOver() const
    {
        return m_RenderIsHovered;
    }

    bool isLeftClicked() const
    {
        return m_RenderIsHovered and m_State.isLeftClickReleasedWithoutDragging;
    }

    bool isRightClicked() const
    {
        return m_RenderIsHovered and m_State.isRightClickReleasedWithoutDragging;
    }

    ModelViewerPanelLayer& pushLayer(std::unique_ptr<ModelViewerPanelLayer> layer)
    {
        // care: do not push new layers directly into `m_Layers`, because `pushLayer` can be
        // called during iteration over `m_Layers` (e.g. during drawing)
        return m_State.pushLayer(std::move(layer));
    }

    void focusOn(const Vector3& position)
    {
        m_Parameters.updRenderParams().camera.focus_point = -position;
    }

    std::optional<Rect> getScreenRect() const
    {
        return m_State.viewportUiRect;
    }

    const PolarPerspectiveCamera& getCamera() const
    {
        return m_Parameters.getRenderParams().camera;
    }

    void setCamera(const PolarPerspectiveCamera& camera)
    {
        m_Parameters.updRenderParams().camera = camera;
    }

    void setModelState(const std::shared_ptr<opyn::ModelStatePair>& newModelState)
    {
        m_Parameters.setModelSharedPtr(newModelState);
    }

    void draw_content()
    {
        // HACK: garbage-collect one frame later, because the layers
        // may have submitted textures to ImGui that are then invalid
        // because GCing destroyed them before they were rendered
        layersGarbageCollect();

        m_State.viewportUiRect = ui::get_content_region_available_ui_rect();
        m_State.isLeftClickReleasedWithoutDragging = ui::is_mouse_released_without_dragging(ui::MouseButton::Left);
        m_State.isRightClickReleasedWithoutDragging = ui::is_mouse_released_without_dragging(ui::MouseButton::Right);

        // if necessary, auto-focus the camera on the first frame
        if (m_IsFirstFrame)
        {
            m_State.updRenderer().autoFocusCamera(
                *m_Parameters.getModelSharedPtr(),
                m_Parameters.updRenderParams(),
                aspect_ratio_of(m_State.viewportUiRect)
            );
            m_IsFirstFrame = false;
        }

        layersOnNewFrame();

        // if the viewer is hovered, handle inputs
        if (m_RenderIsHovered)
        {
            layersHandleMouseInputs();

            if (!ui::wants_keyboard())
            {
                layersHandleKeyboardInputs();
            }
        }

        // render the 3D scene to a texture and present it via a ui::Image
        {
            RenderTexture& sceneTexture = m_State.updRenderer().onDraw(
                *m_Parameters.getModelSharedPtr(),
                m_Parameters.getRenderParams(),
                m_State.viewportUiRect.dimensions(),
                App::settings().get_value<float>("graphics/render_scale", 1.0f) * App::get().main_window_device_pixel_ratio(),
                App::get().anti_aliasing_level()
            );
            ui::draw_image(
                sceneTexture,
                m_State.viewportUiRect.dimensions()
            );

            // care: hittesting is done here, rather than using ui::is_panel_hovered, because
            // we care about whether the _render_ is hovered, not any part of the window (which
            // may include things like the title bar, etc.
            //
            // screwing this up can result in unusual camera behavior, e.g. the camera may move when
            // dragging a visualizer panel around (#739 #93)

            // check if the window is conditionally hovered: this returns true if no other window is
            // overlapping the editor panel, _but_ it also returns true if the user is only hovering
            // the title bar of the window, rather than specifically the render
            const bool windowHovered = ui::is_panel_hovered(ui::HoveredFlag::ChildPanels);

            // check if the 3D render is hovered - ignore blocking and overlapping because the layer
            // stack might be screwing with this
            const bool renderHoveredIgnoringOverlap = ui::is_item_hovered({
                ui::HoveredFlag::AllowWhenBlockedByActiveItem,
                ui::HoveredFlag::AllowWhenOverlapped,
            });

            m_RenderIsHovered = windowHovered && renderHoveredIgnoringOverlap;
        }

        // update state scene AABB
        m_State.maybeSceneVisibleAABB = m_State.getRenderer().visibleBounds();

        // if hovering in 2D, 3D-hittest the scene
        if (m_RenderIsHovered and not (m_State.flags() & ModelViewerPanelFlag::NoHittest))
        {
            m_State.maybeBaseLayerHittest = m_State.getRenderer().getClosestCollision(
                m_Parameters.getRenderParams(),
                ui::get_mouse_ui_position(),
                m_State.viewportUiRect
            );
        }
        else
        {
            m_State.maybeBaseLayerHittest.reset();
        }

        // if there's a 3D-hit, transform it into an OpenSim-hit
        if (m_State.maybeBaseLayerHittest)
        {
            m_State.maybeHoveredComponentAbsPath = OpenSim::ComponentPath{std::string{m_State.maybeBaseLayerHittest->decoration_id}};
        }
        else
        {
            m_State.maybeHoveredComponentAbsPath = {};
        }

        layersDraw();
        layersPopQueuedNewLayers();
    }

private:
    void layersOnNewFrame()
    {
        for (const auto& layerPtr : m_Layers)
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
                (*it)->getFlags() & ModelViewerPanelLayerFlags::CapturesMouseInputs)
            {
                return;
            }
        }
    }

    void layersDraw()
    {
        for (auto it = m_Layers.begin(); it != m_Layers.end(); ++it)
        {
            ModelViewerPanelLayer& layer = **it;

            ui::PanelFlags windowFlags = ui::get_minimal_panel_flags().without(ui::PanelFlag::NoInputs);

            // if any layer above this one captures mouse inputs then disable this layer's inputs
            if (find_if(it+1, m_Layers.end(), [](const auto& layerPtr) -> bool { return layerPtr->getFlags() & ModelViewerPanelLayerFlags::CapturesMouseInputs; }) != m_Layers.end())
            {
                windowFlags |= ui::PanelFlag::NoInputs;
            }

            // layers always have a background (although, it can be entirely invisible)
            windowFlags = windowFlags.without(ui::PanelFlag::NoBackground);
            ui::set_next_panel_bg_alpha(layer.getBackgroundAlpha());

            // draw the layer in a child window, so that ImGui understands that hittests
            // should happen window-by-window (otherwise, you'll have problems with overlapping
            // buttons, widgets, etc.)
            ui::set_next_panel_ui_position(m_State.viewportUiRect.ypd_top_left());
            const std::string childID = std::to_string(std::distance(it, m_Layers.end()));
            if (ui::begin_child_panel(childID, m_State.viewportUiRect.dimensions(), ui::ChildPanelFlags{}, windowFlags)) {
                layer.onDraw(m_Parameters, m_State);
                ui::end_child_panel();
            }
        }
    }

    void layersGarbageCollect()
    {
        std::erase_if(m_Layers, [](const auto& layerPtr)
        {
            return layerPtr->shouldClose();
        });
    }

    void layersPopQueuedNewLayers()
    {
        m_State.flushLayerQueueTo(m_Layers);
    }

    ModelViewerPanelParameters m_Parameters;
    ModelViewerPanelState m_State;
    std::vector<std::unique_ptr<ModelViewerPanelLayer>> m_Layers;
    bool m_IsFirstFrame = true;
    bool m_RenderIsHovered = false;
};


osc::ModelViewerPanel::ModelViewerPanel(
    Widget* parent_,
    std::string_view panelName_,
    const ModelViewerPanelParameters& parameters_,
    ModelViewerPanelFlags flags_) :

    Panel{std::make_unique<Impl>(*this, parent_, panelName_, parameters_, flags_)}
{}
bool osc::ModelViewerPanel::isMousedOver() const { return private_data().isMousedOver(); }
bool osc::ModelViewerPanel::isLeftClicked() const { return private_data().isLeftClicked(); }
bool osc::ModelViewerPanel::isRightClicked() const { return private_data().isRightClicked(); }
ModelViewerPanelLayer& osc::ModelViewerPanel::pushLayer(std::unique_ptr<ModelViewerPanelLayer> layer)
{
    return private_data().pushLayer(std::move(layer));
}
void osc::ModelViewerPanel::focusOn(const Vector3& position) { private_data().focusOn(position); }
std::optional<Rect> osc::ModelViewerPanel::getScreenRect() const { return private_data().getScreenRect(); }
const PolarPerspectiveCamera& osc::ModelViewerPanel::getCamera() const { return private_data().getCamera(); }
void osc::ModelViewerPanel::setCamera(const PolarPerspectiveCamera& camera) { private_data().setCamera(camera); }
void osc::ModelViewerPanel::setModelState(const std::shared_ptr<opyn::ModelStatePair>& newModelState) { private_data().setModelState(newModelState); }
void osc::ModelViewerPanel::impl_draw_content() { private_data().draw_content(); }
void osc::ModelViewerPanel::impl_before_imgui_begin() { ui::push_style_var(ui::StyleVar::PanelPadding, {0.0f, 0.0f}); }
void osc::ModelViewerPanel::impl_after_imgui_begin() { ui::pop_style_var(); }

#include "RendererPerfTestingTab.h"

#include <libopensimcreator/Documents/Model/UndoableModelActions.h>
#include <libopensimcreator/Documents/Model/UndoableModelStatePair.h>
#include <libopensimcreator/Graphics/ModelRendererParams.h>
#include <libopensimcreator/Graphics/OpenSimDecorationGenerator.h>
#include <libopensimcreator/Graphics/OpenSimGraphicsHelpers.h>
#include <libopensimcreator/UI/Shared/BasicWidgets.h>

#include <liboscar/Graphics/Graphics.h>
#include <liboscar/Graphics/Scene/SceneCache.h>
#include <liboscar/Graphics/Scene/SceneHelpers.h>
#include <liboscar/Graphics/Scene/SceneRenderer.h>
#include <liboscar/Graphics/Scene/SceneRendererParams.h>
#include <liboscar/Maths/AABBFunctions.h>
#include <liboscar/Maths/Angle.h>
#include <liboscar/Maths/PolarPerspectiveCamera.h>
#include <liboscar/Maths/Rect.h>
#include <liboscar/Maths/RectFunctions.h>
#include <liboscar/Maths/Vector2.h>
#include <liboscar/Platform/App.h>
#include <liboscar/Platform/AppSettings.h>
#include <liboscar/UI/IconCache.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/UI/Tabs/TabPrivate.h>
#include <liboscar/Utils/CStringView.h>

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

using namespace osc;
using namespace osc::literals;

namespace
{
    struct FrameTimeAccumulator {
        AppClock::duration mean{0.0};
        size_t count = 0;

        void accumulate(AppClock::duration dur)
        {
            mean = (dur + mean*count)/static_cast<double>(count+1);
            ++count;
        }

        float fps() const { return 1.0f/static_cast<float>(mean.count()); }

        void reset() { *this = {}; }
    };
}

class osc::RendererPerfTestingTab::Impl final : public TabPrivate {
public:
    Impl(RendererPerfTestingTab& owner, Widget* parent) :
        TabPrivate{owner, parent, "RendererPerfTesting"}
    {
        generateDecorations();
    }

    void on_mount()
    {
        m_WasVSyncEnabled = App::get().is_vsync_enabled();
        App::upd().set_vsync_enabled(false);
    }

    void on_unmount() const
    {
        App::upd().set_vsync_enabled(m_WasVSyncEnabled);
    }

    void on_tick()
    {
        const auto dt = App::get().frame_delta_since_last_frame();
        if (not m_Paused) {
            m_ModelRendererParams.camera.theta = mod(m_ModelRendererParams.camera.theta + 90_deg*dt.count(), 360_deg);
            m_FrameTimeAccumulator.accumulate(dt);
        }
    }

    void on_draw()
    {
        if (m_RegenerateDecorationsEachFrame) {
            generateDecorations();
        }
        if (std::exchange(m_FirstFrame, false)) {
            if (const std::optional<AABB> sceneAABB = bounding_aabb_of(m_Decorations, &SceneDecoration::world_space_bounds)) {
                auto_focus(m_ModelRendererParams.camera, *sceneAABB);
            }
        }

        const Rect workspaceScreenRect = ui::get_main_window_workspace_screen_space_rect();
        const SceneRendererParams params = calcParams(workspaceScreenRect);
        m_Renderer.render(m_Decorations, params);
        RenderTexture& sceneTexture = m_Renderer.upd_render_texture();
        graphics::blit_to_main_window(sceneTexture, workspaceScreenRect);

        ui::begin_panel("stats");
        ui::draw_checkbox("paused", &m_Paused);
        ui::draw_checkbox("regenerate decorations each frame", &m_RegenerateDecorationsEachFrame);
        ui::draw_text("%f", m_FrameTimeAccumulator.fps());
        ui::same_line();
        if (ui::draw_small_button("reset")) {
            m_FrameTimeAccumulator.reset();
        }
        if (DrawViewerTopButtonRow(m_ModelRendererParams, m_Decorations, *m_IconCache)) {
            generateDecorations();
        }
        ui::end_panel();
    }

private:
    SceneRendererParams calcParams(const Rect& workspaceScreenRect) const
    {
        return CalcSceneRendererParams(
            m_ModelRendererParams,
            workspaceScreenRect.dimensions(),
            App::settings().get_value<float>("graphics/render_scale", 1.0f) * App::get().main_window_device_pixel_ratio(),
            App::get().anti_aliasing_level(),
            1.0f
        );
    }

    void generateDecorations()
    {
        m_Decorations = GenerateModelDecorations(
            m_SceneCache,
            m_Model,
            m_ModelRendererParams.decorationOptions
        );
    }

    bool m_FirstFrame = true;
    bool m_WasVSyncEnabled = false;
    FrameTimeAccumulator m_FrameTimeAccumulator;
    bool m_Paused = false;
    bool m_RegenerateDecorationsEachFrame = false;

    SceneCache m_SceneCache{App::resource_loader()};
    SceneRenderer m_Renderer{m_SceneCache};
    ModelRendererParams m_ModelRendererParams;
    std::vector<SceneDecoration> m_Decorations;

    UndoableModelStatePair m_Model = []()
    {
        UndoableModelStatePair msp{App::resource_filepath("OpenSimCreator/models/RajagopalModel/Rajagopal2016.osim")};
        ActionEnableAllWrappingSurfaces(msp);
        return msp;
    }();

    std::shared_ptr<IconCache> m_IconCache = App::singleton<IconCache>(
        App::resource_loader().with_prefix("OpenSimCreator/icons/"),
        ui::get_font_base_size()/128.0f,
        App::get().highest_device_pixel_ratio()
    );
};

CStringView osc::RendererPerfTestingTab::id() { return "OpenSimCreator/RendererPerfTesting"; }

osc::RendererPerfTestingTab::RendererPerfTestingTab(Widget* parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::RendererPerfTestingTab::impl_on_mount() { private_data().on_mount(); }
void osc::RendererPerfTestingTab::impl_on_unmount() { private_data().on_unmount(); }
void osc::RendererPerfTestingTab::impl_on_tick() { private_data().on_tick(); }
void osc::RendererPerfTestingTab::impl_on_draw() { private_data().on_draw(); }

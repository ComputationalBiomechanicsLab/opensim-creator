#include "SceneViewer.h"

#include <oscar/Graphics/Scene/SceneCache.h>
#include <oscar/Graphics/Scene/SceneDecoration.h>
#include <oscar/Graphics/Scene/SceneRenderer.h>
#include <oscar/Platform/App.h>
#include <oscar/UI/oscimgui.h>

#include <memory>
#include <span>

class osc::SceneViewer::Impl final {
public:

    void onDraw(
        std::span<const SceneDecoration> decorations,
        const SceneRendererParams& renderer_params)
    {
        renderer_.render(decorations, renderer_params);

        // emit the texture to ImGui
        ui::draw_image(renderer_.upd_render_texture(), renderer_.dimensions());
        is_hovered_ = ui::is_item_hovered();
        is_left_clicked_ = ui::is_item_hovered() and ui::is_mouse_released_without_dragging(ImGuiMouseButton_Left);
        is_right_clicked_ = ui::is_item_hovered() and ui::is_mouse_released_without_dragging(ImGuiMouseButton_Right);
    }

    bool is_hovered() const { return is_hovered_; }
    bool is_left_clicked() const { return is_left_clicked_; }
    bool is_right_clicked() const { return is_right_clicked_; }

private:
    SceneRenderer renderer_{*App::singleton<SceneCache>(App::resource_loader())};
    bool is_hovered_ = false;
    bool is_left_clicked_ = false;
    bool is_right_clicked_ = false;
};

osc::SceneViewer::SceneViewer() :
    impl_{std::make_unique<Impl>()}
{}

osc::SceneViewer::SceneViewer(SceneViewer&&) noexcept = default;
osc::SceneViewer& osc::SceneViewer::operator=(SceneViewer&&) noexcept = default;
osc::SceneViewer::~SceneViewer() noexcept = default;

void osc::SceneViewer::on_draw(std::span<const SceneDecoration> decorations, const SceneRendererParams& renderer_params)
{
    impl_->onDraw(decorations, renderer_params);
}

bool osc::SceneViewer::is_hovered() const
{
    return impl_->is_hovered();
}

bool osc::SceneViewer::is_left_clicked() const
{
    return impl_->is_left_clicked();
}

bool osc::SceneViewer::is_right_clicked() const
{
    return impl_->is_right_clicked();
}

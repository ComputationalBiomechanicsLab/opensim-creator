#include "ScreenshotTab.h"

#include <oscar/Formats/Image.h>
#include <oscar/Graphics/Camera.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/ColorSpace.h>
#include <oscar/Graphics/Graphics.h>
#include <oscar/Graphics/Material.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/RenderTexture.h>
#include <oscar/Graphics/Shader.h>
#include <oscar/Graphics/Texture2D.h>
#include <oscar/Graphics/TextureFormat.h>
#include <oscar/Maths/CollisionTests.h>
#include <oscar/Maths/MatFunctions.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>
#include <oscar/Platform/App.h>
#include <oscar/Platform/IconCodepoints.h>
#include <oscar/Platform/os.h>
#include <oscar/Platform/Screenshot.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Tabs/TabPrivate.h>
#include <oscar/Utils/Assertions.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace osc;

namespace
{
    constexpr Color c_unselected_color = Color::white().with_alpha(0.4f);
    constexpr Color c_selected_color = Color::red().with_alpha(0.8f);

    // returns a rect that fully spans at least one dimension of the target rect, but has
    // the given aspect ratio
    //
    // the returned rectangle is in the same space as the target rectangle
    Rect shrink_to_fit(Rect target_rect, float aspect_ratio)
    {
        const float target_aspect_ratio = aspect_ratio_of(target_rect);
        const float ratio = target_aspect_ratio / aspect_ratio;
        const Vec2 target_dimensions = dimensions_of(target_rect);

        if (ratio >= 1.0f) {
            // it will touch the top/bottom but may (ratio != 1.0f) fall short of the left/right
            const Vec2 rv_dimensions = {target_dimensions.x/ratio, target_dimensions.y};
            const Vec2 rv_topleft = {target_rect.p1.x + 0.5f*(target_dimensions.x - rv_dimensions.x), target_rect.p1.y};
            return {rv_topleft, rv_topleft + rv_dimensions};
        }
        else {
            // it will touch the left/right but will not touch the top/bottom
            const Vec2 rv_dimensions = {target_dimensions.x, ratio*target_dimensions.y};
            const Vec2 rv_topleft = {target_rect.p1.x, target_rect.p1.y + 0.5f*(target_dimensions.y - rv_dimensions.y)};
            return {rv_topleft, rv_topleft + rv_dimensions};
        }
    }

    Rect map_rect(const Rect& source_rect, const Rect& target_rect, const Rect& rect)
    {
        const Vec2 scale = dimensions_of(target_rect) / dimensions_of(source_rect);

        return Rect{
            target_rect.p1 + scale*(rect.p1 - source_rect.p1),
            target_rect.p1 + scale*(rect.p2 - source_rect.p1),
        };
    }
}

class osc::ScreenshotTab::Impl final : public TabPrivate {
public:
    explicit Impl(ScreenshotTab& owner, Screenshot&& screenshot) :
        TabPrivate{owner, OSC_ICON_COOKIE " ScreenshotTab"},
        screenshot_{std::move(screenshot)}
    {
        image_texture_.set_filter_mode(TextureFilterMode::Mipmap);
    }

    void on_draw_main_menu()
    {
        if (ui::begin_menu("File")) {
            if (ui::draw_menu_item("Save")) {
                action_try_save_annotated_screenshot();
            }
            ui::end_menu();
        }
    }

    void on_draw()
    {
        ui::enable_dockspace_over_main_viewport();

        // draw screenshot window
        {
            ui::push_style_var(ui::StyleVar::WindowPadding, {0.0f, 0.0f});
            ui::begin_panel("Screenshot");
            ui::pop_style_var();

            const Rect ui_image_rect = draw_screenshot_as_image();
            draw_image_overlays(ui::get_panel_draw_list(), ui_image_rect, c_unselected_color, c_selected_color);

            ui::end_panel();
        }

        // draw controls window
        {
            int id = 0;
            ui::begin_panel("Controls");
            for (const ScreenshotAnnotation& annotation : screenshot_.annotations()) {
                ui::push_id(id++);
                ui::draw_text_unformatted(annotation.label());
                ui::pop_id();
            }
            ui::end_panel();
        }
    }

private:
    // returns screenspace rect of the screenshot within the UI
    Rect draw_screenshot_as_image()
    {
        const Vec2 cursor_topleft = ui::get_cursor_screen_pos();
        const Rect window_rect = {cursor_topleft, cursor_topleft + Vec2{ui::get_content_region_available()}};
        const Rect image_rect = shrink_to_fit(window_rect, aspect_ratio_of(screenshot_.dimensions()));
        ui::set_cursor_screen_pos(image_rect.p1);
        ui::draw_image(image_texture_, dimensions_of(image_rect));
        return image_rect;
    }

    void draw_image_overlays(
        ui::DrawListView drawlist,
        const Rect& image_rect,
        const Color& unselected_color,
        const Color& selected_color)
    {
        const Vec2 mouse_pos = ui::get_mouse_pos();
        const bool left_click_released = ui::is_mouse_released(ui::MouseButton::Left);
        const Rect image_source_rect = {{0.0f, 0.0f}, screenshot_.dimensions()};

        for (const ScreenshotAnnotation& annotation : screenshot_.annotations()) {
            const Rect annotation_rect_screen = map_rect(image_source_rect, image_rect, annotation.rect());
            const bool selected = user_selected_annotations_.contains(annotation.label());
            const bool hovered = is_intersecting(annotation_rect_screen, mouse_pos);

            Color color = selected ? selected_color : unselected_color;
            if (hovered) {
                color.a = saturate(color.a + 0.3f);
            }

            if (hovered and left_click_released) {
                if (selected) {
                    user_selected_annotations_.erase(annotation.label());
                }
                else {
                    user_selected_annotations_.insert(annotation.label());
                }
            }

            drawlist.add_rect(
                annotation_rect_screen,
                color,
                3.0f,
                3.0f
            );
        }
    }

    void action_try_save_annotated_screenshot()
    {
        const std::optional<std::filesystem::path> maybe_image_path =
            prompt_user_for_file_save_location_add_extension_if_necessary("png");

        if (maybe_image_path) {
            std::ofstream fout{*maybe_image_path, std::ios_base::binary};
            if (not fout) {
                throw std::runtime_error{maybe_image_path->string() + ": cannot open for writing"};
            }
            const Texture2D annotated_screenshot = render_annotated_screenshot();
            write_to_png(annotated_screenshot, fout);
            open_file_in_os_default_application(*maybe_image_path);
        }
    }

    Texture2D render_annotated_screenshot()
    {
        RenderTexture render_texture{{.dimensions = image_texture_.dimensions()}};

        // blit the screenshot into the output
        graphics::blit(image_texture_, render_texture);

        // draw overlays to a local ImGui drawlist
        ui::DrawList drawlist;
        Color outline_color = c_selected_color;
        outline_color.a = 1.0f;
        draw_image_overlays(
            drawlist,
            Rect{{0.0f, 0.0f}, image_texture_.dimensions()},
            {0.0f, 0.0f, 0.0f, 0.0f},
            outline_color
        );

        // render drawlist to output
        drawlist.render_to(render_texture);

        Texture2D rv{render_texture.dimensions(), TextureFormat::RGB24, ColorSpace::sRGB};
        graphics::copy_texture(render_texture, rv);
        return rv;
    }

    Screenshot screenshot_;
    Texture2D image_texture_ = screenshot_.image();
    std::unordered_set<std::string> user_selected_annotations_;
};

osc::ScreenshotTab::ScreenshotTab(Widget&, Screenshot&& screenshot) :
    Tab{std::make_unique<Impl>(*this, std::move(screenshot))}
{}
void osc::ScreenshotTab::impl_on_draw_main_menu() { private_data().on_draw_main_menu(); }
void osc::ScreenshotTab::impl_on_draw() { private_data().on_draw(); }

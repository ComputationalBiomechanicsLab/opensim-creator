#include "screenshot_tab.h"

#include <liboscar/formats/image.h>
#include <liboscar/graphics/Color.h>
#include <liboscar/graphics/ColorSpace.h>
#include <liboscar/graphics/Graphics.h>
#include <liboscar/graphics/Material.h>
#include <liboscar/graphics/RenderTexture.h>
#include <liboscar/graphics/Texture2D.h>
#include <liboscar/graphics/TextureFormat.h>
#include <liboscar/maths/CollisionTests.h>
#include <liboscar/maths/MathHelpers.h>
#include <liboscar/maths/Rect.h>
#include <liboscar/maths/RectFunctions.h>
#include <liboscar/maths/Vector2.h>
#include <liboscar/maths/Vector3.h>
#include <liboscar/maths/Vector4.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/os.h>
#include <liboscar/platform/screenshot.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/tabs/tab_private.h>
#include <liboscar/utils/EnumHelpers.h>
#include <liboscar/utils/StringHelpers.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>

using namespace osc;

namespace
{
    constexpr Color c_unselected_color = Color::white().with_alpha(0.4f);
    constexpr Color c_selected_color = Color::red().with_alpha(0.8f);
    constexpr float c_annotation_rect_rounding = 3.0f;
    constexpr float c_annotation_rect_thickness = 3.0f;

    // returns a rect that fully spans at least one dimension of the target rect, but has
    // the given aspect ratio
    //
    // the returned rectangle is in the same space as the target rectangle
    Rect shrink_to_fit(Rect target_ui_rect, float aspect_ratio)
    {
        const float target_aspect_ratio = aspect_ratio_of(target_ui_rect);
        const float ratio = target_aspect_ratio / aspect_ratio;
        const Vector2 target_dimensions = target_ui_rect.dimensions();
        const Vector2 target_ui_top_left = target_ui_rect.ypd_top_left();

        if (ratio >= 1.0f) {
            // it will touch the top/bottom but may (ratio != 1.0f) fall short of the left/right
            const Vector2 rv_dimensions = {target_dimensions.x/ratio, target_dimensions.y};
            const Vector2 rv_top_left = {target_ui_top_left.x + 0.5f*(target_dimensions.x - rv_dimensions.x), target_ui_top_left.y};
            return Rect::from_corners(rv_top_left, rv_top_left + rv_dimensions);
        }
        else {
            // it will touch the left/right but will not touch the top/bottom
            const Vector2 rv_dimensions = {target_dimensions.x, ratio*target_dimensions.y};
            const Vector2 rv_top_left = {target_ui_top_left.x, target_ui_top_left.y + 0.5f*(target_dimensions.y - rv_dimensions.y)};
            return Rect::from_corners(rv_top_left, rv_top_left + rv_dimensions);
        }
    }

    Rect map_rect(
        const Vector2& screen_dimensions,
        const Rect& annotation_screen_rect,
        const Rect& viewport_ui_rect)
    {
        const auto annotation_screen_rect_corners = annotation_screen_rect.corners();

        const Rect normalized_ypu_rect = Rect::from_corners(
            annotation_screen_rect_corners.min/screen_dimensions,
            annotation_screen_rect_corners.max/screen_dimensions
        );
        const Rect normalized_ypd_rect = normalized_ypu_rect.with_flipped_y(1.0f);
        const auto normalized_ypd_corners = normalized_ypd_rect.corners();

        const Vector2 ui_dims = viewport_ui_rect.dimensions();
        const Vector2 ui_top_left = viewport_ui_rect.ypd_top_left();
        return Rect::from_corners(
            ui_top_left + ui_dims*normalized_ypd_corners.min,
            ui_top_left + ui_dims*normalized_ypd_corners.max
        );
    }

    enum class ScreenshotFileFormat { png, jpeg, NUM_OPTIONS };
}

class osc::ScreenshotTab::Impl final : public TabPrivate {
public:
    explicit Impl(ScreenshotTab& owner, Widget* parent, Screenshot&& screenshot) :
        TabPrivate{owner, parent, "Screenshot"},
        screenshot_{std::move(screenshot)}
    {
        image_texture_.set_filter_mode(TextureFilterMode::Mipmap);
    }

    void on_draw_main_menu()
    {
        if (ui::begin_menu("File")) {
            if (ui::draw_menu_item("Save PNG")) {
                action_try_save_annotated_screenshot(ScreenshotFileFormat::png);
            }
            if (ui::draw_menu_item("Save JPEG")) {
                action_try_save_annotated_screenshot(ScreenshotFileFormat::jpeg);
            }
            ui::draw_float_circular_slider("JPEG quality", &jpeg_quality_level_, 0.0f, 1.0f);
            ui::end_menu();
        }
    }

    void on_draw()
    {
        ui::enable_dockspace_over_main_window();

        // draw screenshot window
        {
            ui::push_style_var(ui::StyleVar::PanelPadding, {0.0f, 0.0f});
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

            // show editor for setting window size
            {
                const Vector2 s = App::get().main_window_dimensions();
                ui::draw_text("%f %f", s.x, s.y);
                if (ui::draw_button("change")) {
                    App::upd().try_async_set_main_window_dimensions({1920.0f, 1080.0f});
                }
            }

            if (ui::begin_table("##Annotations", 2)) {
                ui::table_setup_column("Label");
                ui::table_setup_column("Screen Position");
                ui::table_headers_row();
                ui::table_next_row();
                for (const ScreenshotAnnotation& annotation : screenshot_.annotations()) {
                    ui::push_id(id++);
                    ui::table_set_column_index(0);
                    ui::draw_text(annotation.label());
                    ui::table_set_column_index(1);
                    ui::draw_text(stream_to_string(annotation.rect()));
                    ui::table_next_row();
                    ui::pop_id();
                }
                ui::end_table();
            }


            ui::end_panel();
        }
    }

private:
    // returns ui space rect of the screenshot within the UI
    Rect draw_screenshot_as_image()
    {
        const Rect window_ui_rect = ui::get_content_region_available_ui_rect();
        const Rect image_ui_rect = shrink_to_fit(window_ui_rect, aspect_ratio_of(screenshot_.dimensions()));
        ui::set_cursor_ui_position(image_ui_rect.ypd_top_left());
        ui::draw_image(image_texture_, image_ui_rect.dimensions());
        return image_ui_rect;
    }

    void draw_image_overlays(
        ui::DrawListView draw_list,
        const Rect& image_ui_rect,
        const Color& unselected_color,
        const Color& selected_color)
    {
        const Vector2 mouse_ui_pos = ui::get_mouse_ui_position();
        const bool left_click_released = ui::is_mouse_released(ui::MouseButton::Left);
        const Vector2 screenshot_dimensions = screenshot_.dimensions();

        for (const ScreenshotAnnotation& annotation : screenshot_.annotations()) {
            const Rect annotation_ui_rect = map_rect(screenshot_dimensions, annotation.rect(), image_ui_rect);
            const bool selected = user_selected_annotations_.contains(annotation.label());
            const bool hovered = is_intersecting(annotation_ui_rect, mouse_ui_pos);

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

            draw_list.add_rect(
                annotation_ui_rect,
                color,
                c_annotation_rect_rounding,
                c_annotation_rect_thickness
            );
        }
    }

    void action_try_save_annotated_screenshot(ScreenshotFileFormat format)
    {
        App::upd().prompt_user_to_save_file_with_extension_async([format, screenshot = render_annotated_screenshot(), jpeg_quality_level=jpeg_quality_level_](std::optional<std::filesystem::path> p)
        {
            if (not p) {
                return;  // User cancelled out.
            }

            std::ofstream fout{*p, std::ios_base::binary};
            if (not fout) {
                throw std::runtime_error{p->string() + ": cannot open for writing"};
            }
            switch (format) {
            case ScreenshotFileFormat::jpeg: JPEG::write(fout, screenshot, jpeg_quality_level); break;
            case ScreenshotFileFormat::png:  PNG::write(fout, screenshot);  break;
            default:                         std::unreachable();
            }
            open_file_in_os_default_application(*p);
        }, format == ScreenshotFileFormat::jpeg ? "jpeg" : "png");
        static_assert(num_options<ScreenshotFileFormat>() == 2);
    }

    Texture2D render_annotated_screenshot()
    {
        RenderTexture render_texture{{
            .pixel_dimensions = image_texture_.pixel_dimensions(),
            .device_pixel_ratio = image_texture_.device_pixel_ratio(),
        }};

        // blit the screenshot into the output
        graphics::blit(image_texture_, render_texture);

        // draw overlays to a local ImGui draw list
        ui::DrawList draw_list;
        draw_list.push_clip_rect(Rect::from_corners({}, image_texture_.dimensions()));

        draw_image_overlays(
            draw_list,
            Rect::from_corners({}, image_texture_.dimensions()),
            Color::clear(),
            c_selected_color.with_alpha(1.0f)
        );

        // render draw list to output
        draw_list.render_to(render_texture);
        draw_list.pop_clip_rect();

        Texture2D rv{render_texture.pixel_dimensions(), TextureFormat::RGB24, ColorSpace::sRGB};
        graphics::copy_texture(render_texture, rv);
        return rv;
    }

    Screenshot screenshot_;
    Texture2D image_texture_ = screenshot_.texture();
    std::unordered_set<std::string> user_selected_annotations_;
    float jpeg_quality_level_ = 0.7f;
};

osc::ScreenshotTab::ScreenshotTab(Widget* parent, Screenshot&& screenshot) :
    Tab{std::make_unique<Impl>(*this, parent, std::move(screenshot))}
{}
void osc::ScreenshotTab::impl_on_draw_main_menu() { private_data().on_draw_main_menu(); }
void osc::ScreenshotTab::impl_on_draw() { private_data().on_draw(); }

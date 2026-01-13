#include "ImGuizmoDemoTab.h"

#include <liboscar/graphics/Camera.h>
#include <liboscar/graphics/geometries/GridGeometry.h>
#include <liboscar/graphics/geometries/PlaneGeometry.h>
#include <liboscar/graphics/Graphics.h>
#include <liboscar/graphics/materials/MeshBasicMaterial.h>
#include <liboscar/maths/Matrix4x4.h>
#include <liboscar/maths/MatrixFunctions.h>
#include <liboscar/maths/PolarPerspectiveCamera.h>
#include <liboscar/maths/QuaternionFunctions.h>
#include <liboscar/maths/Rect.h>
#include <liboscar/maths/RectFunctions.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/tabs/TabPrivate.h>

#include <memory>

using namespace osc::literals;
using namespace osc;

class osc::ImGuizmoDemoTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "oscar_demos/ImGuizmo"; }

    explicit Impl(ImGuizmoDemoTab& owner, Widget* parent) :
        TabPrivate{owner, parent, static_label()}
    {
        basic_material_.set_transparent(true);
    }

    void on_draw()
    {
        const Matrix4x4 view_matrix = scene_camera_.view_matrix();
        const Rect workspace_screen_space_rect = ui::get_main_window_workspace_screen_space_rect();
        const Matrix4x4 projection_matrix = scene_camera_.projection_matrix(aspect_ratio_of(workspace_screen_space_rect));

        // Render 3D scene: a grid floor and a cube that has a different color per face
        {
            Camera render_camera;
            render_camera.set_view_matrix_override(view_matrix);
            render_camera.set_projection_matrix_override(projection_matrix);
            render_camera.set_pixel_rect(workspace_screen_space_rect);

            for (size_t i = 0; i < 6; ++i) {
                // axis-aligned vector
                Vector3 v;
                v[i % 3] = i/3 ? -1.0f : 1.0f;

                const Matrix4x4 xform = model_matrix_ * translate(identity<Matrix4x4>(), 0.5f*v) * matrix4x4_cast(rotation(plane_.normal(), v));
                const Color color = Color{0.4f}.with_element(i % 3, 0.8f);
                graphics::draw(
                    plane_.mesh(),
                    xform,
                    basic_material_,
                    render_camera,
                    MeshBasicMaterial::PropertyBlock{color}
                );
            }
            basic_material_.set_color(Color::white());
            graphics::draw(
                grid_,
                {.rotation = rotation(grid_.normal(), {0.0f, 1.0f, 0.0f})},
                basic_material_,
                render_camera,
                MeshBasicMaterial::PropertyBlock{Color::white().with_alpha(0.1f)}
            );
            render_camera.render_to_main_window();
        }

        // Draw UI overlays (incl. gizmo)
        gizmo_.draw_to_foreground(
            model_matrix_,
            view_matrix,
            projection_matrix,
            ui::get_main_window_workspace_ui_rect()
        );
        ui::draw_gizmo_mode_selector(gizmo_);
        ui::draw_gizmo_operation_selector(gizmo_);
    }

private:
    PolarPerspectiveCamera scene_camera_ = []()
    {
        PolarPerspectiveCamera rv;
        rv.focus_point = {0.0f, 0.0f, 0.0f};
        rv.phi = 1_rad;
        rv.theta = 0_rad;
        rv.radius = 5.0f;
        return rv;
    }();

    ui::Gizmo gizmo_;
    Matrix4x4 model_matrix_ = identity<Matrix4x4>();
    GridGeometry grid_{{.size = 20.0f, .num_divisions = 100}};
    PlaneGeometry plane_;
    MeshBasicMaterial basic_material_;
};


CStringView osc::ImGuizmoDemoTab::id() { return Impl::static_label(); }

osc::ImGuizmoDemoTab::ImGuizmoDemoTab(Widget* parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::ImGuizmoDemoTab::impl_on_draw() { private_data().on_draw(); }

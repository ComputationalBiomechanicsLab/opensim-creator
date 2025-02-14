#include "BookOfShadersTab.h"

#include <liboscar/oscar.h>

#include <chrono>
#include <memory>
#include <vector>

using namespace osc;

namespace
{
    constexpr CStringView c_basic_vertex_shader = R"(
#version 330 core

uniform mat4 uViewProjMat;

layout (location = 0) in vec3 aPos;
layout (location = 6) in mat4 aModelMat;

void main()
{
    gl_Position = uViewProjMat * aModelMat * vec4(aPos, 1.0);
}
)";

    class BookOfShadersCommonProperties final : public MaterialPropertyBlock {
    public:
        void set_time(AppClock::time_point p) { set("u_time", std::chrono::duration_cast<std::chrono::duration<float>>(p.time_since_epoch()).count()); }
        void set_resolution(Vec2 resolution) { set("u_resolution", resolution); }
        void set_mouse_position(Vec2 mouse_position) { set("u_mouse", mouse_position); }
    };

    class BookOfShadersMaterial : public Material {
    public:
        explicit BookOfShadersMaterial(CStringView name, CStringView fragment_shader_src) :
            Material{Shader{c_basic_vertex_shader, fragment_shader_src}},
            name_{name}
        {}

        CStringView name() const { return name_; }
    private:
        std::string name_;
    };

    class HelloWorldMaterial final : public BookOfShadersMaterial {
    public:
        HelloWorldMaterial() : BookOfShadersMaterial{"hello_world", c_fragment_source} {}
    private:
        static constexpr CStringView c_fragment_source = R"(
#version 330 core

out vec4 FragColor;

void main() {
    FragColor = vec4(1.0, 0.0, 1.0, 1.0);
}
)";
    };

    class Uniforms_TimeColored final : public BookOfShadersMaterial {
    public:
        Uniforms_TimeColored() : BookOfShadersMaterial{"uniforms_time_colored", c_fragment_source} {}
    private:
        static constexpr CStringView c_fragment_source = R"(
#version 330 core

uniform float u_time;

out vec4 FragColor;

void main() {
    FragColor = vec4(abs(sin(u_time)), 0.0, 0.0, 1.0);
}
)";
    };

    class Uniforms_gl_FragCoord final : public BookOfShadersMaterial {
    public:
        Uniforms_gl_FragCoord() : BookOfShadersMaterial{"uniforms_gl_FragCoord", c_fragment_source} {}
    private:
        static constexpr CStringView c_fragment_source = R"(
#version 330 core

uniform vec2 u_resolution;

out vec4 FragColor;

void main() {
    vec2 st = gl_FragCoord.xy/u_resolution;
    FragColor = vec4(st.x, st.y, 0.0, 1.0);
}
)";
    };

    class AlgorithmicDrawing_Smoothstep final : public BookOfShadersMaterial {
    public:
        AlgorithmicDrawing_Smoothstep() : BookOfShadersMaterial{"algorithmic_drawing", c_fragment_source} {}
    private:
        static constexpr CStringView c_fragment_source = R"(
#version 330 core

uniform vec2 u_resolution;

out vec4 FragColor;

float plot(vec2 st) {
    return smoothstep(0.0, 0.02, 0.02 - abs(st.y - st.x));
}

void main() {
    vec2 st = gl_FragCoord.xy/u_resolution;

    float y = st.x;

    // note: BookOfShaders works with sRGB colors
    vec3 color = vec3(pow(y, 2.2));

    // Plot a line
    float pct = plot(st);
    color = mix(color, vec3(0.0, 1.0, 0.0), pct);

    FragColor = vec4(color,1.0);
}
)";
    };
}

class osc::BookOfShadersTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "oscar_demos/bookofshaders/All"; }

    explicit Impl(BookOfShadersTab& owner, Widget* parent) :
        TabPrivate{owner, parent, static_label()}
    {
        camera_.set_projection(CameraProjection::Orthographic);
        camera_.set_clipping_planes({-1.0f, 1.0f});
        camera_.set_direction({0.0f, 0.0f, 1.0f});
        camera_.set_orthographic_size(1.0f);
    }

    void on_draw()
    {
        render_example_to_screen();
        draw_2d_ui();
    }

private:
    void render_example_to_screen()
    {
        // update properties for this frame
        const Vec2 workspace_dimensions = ui::get_main_viewport_workspace_screen_dimensions();
        props_.set_time(App::get().frame_start_time());
        props_.set_resolution(workspace_dimensions * App::get().main_window_device_pixel_ratio());
        props_.set_mouse_position(ui::get_mouse_pos());

        graphics::draw(
            quad_,
            {.scale = {aspect_ratio_of(workspace_dimensions), 1.0f, 1.0f}},
            materials_.at(current_material_index_),
            camera_,
            props_
        );
        camera_.render_to_screen();
    }

    void draw_2d_ui()
    {
        ui::begin_panel("material selector");
        for (size_t i = 0; i < materials_.size(); ++i) {
            if (ui::draw_button(materials_[i].name())) {
                current_material_index_ = i;
            }
        }
        ui::end_panel();
    }

    std::vector<BookOfShadersMaterial> materials_ = {
        HelloWorldMaterial{},
        Uniforms_TimeColored{},
        Uniforms_gl_FragCoord{},
        AlgorithmicDrawing_Smoothstep{},
    };
    size_t current_material_index_ = 0;
    PlaneGeometry quad_;
    Camera camera_;
    BookOfShadersCommonProperties props_;
};

CStringView osc::BookOfShadersTab::id() { return Impl::static_label(); }

osc::BookOfShadersTab::BookOfShadersTab(Widget* parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::BookOfShadersTab::impl_on_draw() { private_data().on_draw(); }

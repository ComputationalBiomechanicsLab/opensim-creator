#include "BookOfShadersTab.h"

#include <oscar/oscar.h>

#include <memory>
#include <vector>

using namespace osc;

namespace
{
    constexpr CStringView c_tab_string_id = "BookOfShaders/All";

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

void main() {
    gl_FragColor = vec4(1.0, 0.0, 1.0, 1.0);
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

void main() {
    gl_FragColor = vec4(abs(sin(u_time)), 0.0, 0.0, 1.0);
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

void main() {
    vec2 st = gl_FragCoord.xy/u_resolution;
    gl_FragColor = vec4(st.x, st.y, 0.0, 1.0);
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

    gl_FragColor = vec4(color,1.0);
}
)";
    };
}

class osc::BookOfShadersTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_tab_string_id}
    {
        camera_.set_projection(CameraProjection::Orthographic);
        camera_.set_clipping_planes({-1.0f, 1.0f});
        camera_.set_direction({0.0f, 0.0f, 1.0f});
        camera_.set_orthographic_size(1.0f);
    }

private:
    void impl_on_draw() final
    {
        render_example_to_screen();
        draw_2d_ui();
    }

    void render_example_to_screen()
    {
        // update properties for this frame
        const Vec2 workspace_dimensions = ui::get_main_viewport_workspace_screen_dimensions();
        props_.set_time(App::get().frame_start_time());
        props_.set_resolution(workspace_dimensions);
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

CStringView osc::BookOfShadersTab::id()
{
    return c_tab_string_id;
}

osc::BookOfShadersTab::BookOfShadersTab(const ParentPtr<ITabHost>&) :
    impl_{std::make_unique<Impl>()}
{}
osc::BookOfShadersTab::BookOfShadersTab(BookOfShadersTab&&) noexcept = default;
osc::BookOfShadersTab& osc::BookOfShadersTab::operator=(BookOfShadersTab&&) noexcept = default;
osc::BookOfShadersTab::~BookOfShadersTab() noexcept = default;

UID osc::BookOfShadersTab::impl_get_id() const
{
    return impl_->id();
}

CStringView osc::BookOfShadersTab::impl_get_name() const
{
    return impl_->name();
}

void osc::BookOfShadersTab::impl_on_mount()
{
    impl_->on_mount();
}

void osc::BookOfShadersTab::impl_on_unmount()
{
    impl_->on_unmount();
}

bool osc::BookOfShadersTab::impl_on_event(const SDL_Event& e)
{
    return impl_->on_event(e);
}

void osc::BookOfShadersTab::impl_on_draw()
{
    impl_->on_draw();
}

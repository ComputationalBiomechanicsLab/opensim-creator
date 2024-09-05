#include "BookOfShadersTab.h"

#include <oscar/oscar.h>

#include <map>
#include <memory>

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

    class BookOfShadersShader : public Material {
    public:
        explicit BookOfShadersShader(CStringView fragment_shader_src) :
            Material{Shader{c_basic_vertex_shader, fragment_shader_src}}
        {}
    };

    class HelloWorldMaterial final : public BookOfShadersShader {
    public:
        HelloWorldMaterial() : BookOfShadersShader{c_fragment_source} {}
    private:
        static constexpr CStringView c_fragment_source = R"(
#version 330 core

void main() {
    gl_FragColor = vec4(1.0, 0.0, 1.0, 1.0);
}
)";
    };

    class Uniforms_TimeColored final : public BookOfShadersShader {
    public:
        Uniforms_TimeColored() : BookOfShadersShader{c_fragment_source} {}
    private:
        static constexpr CStringView c_fragment_source = R"(
#version 330 core

uniform float u_time;

void main() {
    gl_FragColor = vec4(abs(sin(u_time)), 0.0, 0.0, 1.0);
}
)";
    };

    class Uniforms_gl_FragCoord final : public BookOfShadersShader {
    public:
        Uniforms_gl_FragCoord() : BookOfShadersShader{c_fragment_source} {}
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

    class AlgorithmicDrawing_LinePlot final : public BookOfShadersShader {
    public:
        AlgorithmicDrawing_LinePlot() : BookOfShadersShader{c_fragment_source} {}
    private:
        static constexpr CStringView c_fragment_source = R"(
#version 330 core

uniform vec2 u_resolution;

float plot(vec2 st) {
    return smoothstep(0.0, 0.2, 0.2 - abs(st.y - st.x));
}

void main() {
	vec2 st = gl_FragCoord.xy/u_resolution;

    float y = st.x;

    vec3 color = vec3(y);

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
        {
            props_.set_time(App::get().frame_start_time());
            props_.set_resolution(ui::get_main_viewport_workspace_screen_dimensions());
            props_.set_mouse_position(ui::get_mouse_pos());
        }

        graphics::draw(quad_, identity<Transform>(), materials_.find(current_material_)->second, camera_, props_);
        camera_.render_to_screen();
    }

    void draw_2d_ui()
    {
        ui::begin_panel("material selector");
        for (const auto& [name, _] : materials_) {
            if (ui::draw_button(name)) {
                current_material_ = name;
            }
        }
        ui::end_panel();
    }

    std::map<std::string, Material> materials_ = {
        {"hello_world", HelloWorldMaterial{}},
        {"uniforms_time_colored", Uniforms_TimeColored{}},
        {"uniforms_gl_FragCoord", Uniforms_gl_FragCoord{}},
        {"algorithmic_drawing", AlgorithmicDrawing_LinePlot{}},
    };
    std::string current_material_ = materials_.begin()->first;
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

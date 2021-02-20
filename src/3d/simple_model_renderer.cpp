#include "simple_model_renderer.hpp"

#include "raw_renderer.hpp"

#include "3d_common.hpp"
#include "model_drawlist_generator.hpp"
#include "src/application.hpp"
#include "src/sdl_wrapper.hpp"

#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/Muscle.h>
#include <SDL_events.h>
#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <SDL_mouse.h>
#include <SDL_video.h>
#include <SimTKcommon.h>
#include <SimTKcommon/Orientation.h>
#include <SimTKsimbody.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

void osmv::apply_standard_rim_coloring(
    Labelled_model_drawlist& drawlist, OpenSim::Component const* hovered, OpenSim::Component const* selected) {

    if (selected == nullptr) {
        // replace with a senteniel because nullptr means "not assigned"
        // in the geometry list
        selected = reinterpret_cast<OpenSim::Component const*>(-1);
    }

    drawlist.for_each([selected, hovered](OpenSim::Component const* owner, Raw_mesh_instance& mi) {
        unsigned char rim_alpha;
        if (owner == selected) {
            rim_alpha = 255;
        } else if (hovered != nullptr and hovered == owner) {
            rim_alpha = 70;
        } else {
            rim_alpha = 0;
        }

        mi.set_rim_alpha(rim_alpha);
    });
}

namespace osmv {
    struct Simple_model_renderer_impl final {
        Raw_renderer renderer;
        Model_drawlist_generator drawlist_generator;

        Simple_model_renderer_impl(int w, int h, int samples) : renderer{osmv::Raw_renderer_config{w, h, samples}} {
        }
    };
}

osmv::Simple_model_renderer::Simple_model_renderer(int w, int h, int samples) :
    impl{new Simple_model_renderer_impl{w, h, samples}} {

    OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();
}

osmv::Simple_model_renderer::~Simple_model_renderer() noexcept {
    delete impl;
}

void osmv::Simple_model_renderer::reallocate_buffers(int w, int h, int samples) {
    impl->renderer.change_config(Raw_renderer_config{w, h, samples});
}

bool osmv::Simple_model_renderer::on_event(SDL_Event const& e) {
    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
        case SDLK_w:
            flags ^= SimpleModelRendererFlags_WireframeMode;
            return true;
        }
    } else if (e.type == SDL_MOUSEBUTTONDOWN) {
        switch (e.button.button) {
        case SDL_BUTTON_LEFT:
            camera.on_left_click_down();
            return true;
        case SDL_BUTTON_RIGHT:
            camera.on_right_click_down();
            return true;
        }
    } else if (e.type == SDL_MOUSEBUTTONUP) {
        switch (e.button.button) {
        case SDL_BUTTON_LEFT:
            camera.on_left_click_up();
            return true;
        case SDL_BUTTON_RIGHT:
            camera.on_right_click_up();
            return true;
        }
    } else if (e.type == SDL_MOUSEMOTION) {
        glm::vec2 d = impl->renderer.dimensions();
        float aspect_ratio = d.x / d.y;
        float dx = static_cast<float>(e.motion.xrel) / d.x;
        float dy = static_cast<float>(e.motion.yrel) / d.y;
        camera.on_mouse_motion(aspect_ratio, dx, dy);
    } else if (e.type == SDL_MOUSEWHEEL) {
        if (e.wheel.y > 0) {
            camera.on_scroll_up();
        } else {
            camera.on_scroll_down();
        }
        return true;
    }
    return false;
}

void osmv::Simple_model_renderer::generate_geometry(OpenSim::Model const& model, SimTK::State const& state) {
    // iterate over all components in the OpenSim model, keeping a few things in mind:
    //
    // - Anything in the component tree *might* render geometry
    //
    // - For selection logic, we only (currently) care about certain high-level components,
    //   like muscles
    //
    // - Pretend the component tree traversal is implementation-defined because OpenSim's
    //   implementation of component-tree walking is a bit of a clusterfuck. At time of
    //   writing, it's a breadth-first recursive descent
    //
    // - Components of interest, like muscles, might not render their geometry - it might be
    //   delegated to a subcomponent
    //
    // So this algorithm assumes that the list iterator is arbitrary, but always returns
    // *something* in a tree that has the current model as a root. So, for each component that
    // pops out of `getComponentList`, crawl "up" to the root. If we encounter something
    // interesting (e.g. a `Muscle`) then we tag the geometry against that component, rather
    // than the component that is rendering.

    geometry.clear();

    auto on_append = [&](ModelDrawlistOnAppendFlags f, OpenSim::Component const*& c, Raw_mesh_instance&) {
        if (f & ModelDrawlistOnAppendFlags_IsStatic and
            not(flags & SimpleModelRendererFlags_HoverableStaticDecorations)) {
            c = nullptr;
        }

        if (f & ModelDrawlistOnAppendFlags_IsDynamic and
            not(flags & SimpleModelRendererFlags_HoverableDynamicDecorations)) {
            c = nullptr;
        }
    };

    ModelDrawlistGeneratorFlags draw_flags = ModelDrawlistGeneratorFlags_None;
    if (flags & SimpleModelRendererFlags_DrawStaticDecorations) {
        draw_flags |= ModelDrawlistGeneratorFlags_GenerateStaticDecorations;
    }
    if (flags & SimpleModelRendererFlags_DrawDynamicDecorations) {
        draw_flags |= ModelDrawlistGeneratorFlags_GenerateDynamicDecorations;
    }

    impl->drawlist_generator.generate(model, state, geometry, on_append, draw_flags);

    geometry.optimize();
}

gl::Texture_2d& osmv::Simple_model_renderer::draw() {
    Raw_drawcall_params params;
    params.passthrough_hittest_x = hovertest_x;
    params.passthrough_hittest_y = hovertest_y;
    params.view_matrix = camera.view_matrix();
    params.projection_matrix = camera.projection_matrix(impl->renderer.aspect_ratio());
    params.view_pos = camera.pos();
    params.light_pos = light_pos;
    params.light_rgb = light_rgb;
    params.background_rgba = background_rgba;
    params.rim_rgba = rim_rgba;
    params.rim_thickness = rim_thickness;
    params.flags = RawRendererFlags_None;
    params.flags |= RawRendererFlags_PerformPassthroughHitTest;
    params.flags |= RawRendererFlags_UseOptimizedButDelayed1FrameHitTest;
    params.flags |= RawRendererFlags_DrawSceneGeometry;
    if (flags & SimpleModelRendererFlags_WireframeMode) {
        params.flags |= RawRendererFlags_WireframeMode;
    }
    if (flags & SimpleModelRendererFlags_ShowMeshNormals) {
        params.flags |= RawRendererFlags_ShowMeshNormals;
    }
    if (flags & SimpleModelRendererFlags_ShowFloor) {
        params.flags |= RawRendererFlags_ShowFloor;
    }
    if (flags & SimpleModelRendererFlags_DrawRims) {
        params.flags |= RawRendererFlags_DrawRims;
    }
    if (app().is_in_debug_mode()) {
        params.flags |= RawRendererFlags_DrawDebugQuads;
    }

    // perform draw call
    Raw_drawcall_result result = impl->renderer.draw(params, geometry.raw_drawlist());

    // post-draw: check if the hit-test passed
    // TODO:: optimized indices are from the previous frame, which might
    //        contain now-stale components
    hovered_component = geometry.component_from_passthrough(result.passthrough_result);

    return result.texture;
}

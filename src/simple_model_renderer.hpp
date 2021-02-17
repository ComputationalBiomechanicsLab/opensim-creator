#pragma once

#include "raw_renderer.hpp"

#include <SDL_events.h>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <vector>

namespace OpenSim {
    class Component;
}
namespace OpenSim {
    class Model;
}
namespace SimTK {
    class State;
}
namespace osmv {
    class Application;
}
namespace osmv {
    struct Simple_model_renderer_impl;
}

namespace osmv {
    // geometry generated from an OpenSim model + SimTK state pair
    struct OpenSim_model_geometry final {
        // these two vectors are 1:1 associated
        Raw_renderer_drawlist drawlist;
        std::vector<OpenSim::Component const*> associated_components;

        void clear() {
            drawlist.clear();
            associated_components.clear();
        }

        Mesh_instance& push_back(OpenSim::Component const*, glm::mat4 transform, glm::vec4 rgba, int meshid);
    };

    struct Mutable_opensim_mesh_instance final {
        OpenSim::Component const*& associated_component;
        Mesh_instance& data;
    };

    struct Immutable_opensim_mesh_instance final {
        OpenSim::Component const* associated_component;
        Mesh_instance const& data;
    };

    template<bool IsConst>
    class OpenSim_geometry_iterator final {
        OpenSim_model_geometry& geom;
        size_t pos = 0;

    public:
        OpenSim_geometry_iterator(OpenSim_model_geometry& _geom, size_t _pos) noexcept : geom{_geom}, pos{_pos} {
        }

        template<bool T = IsConst, typename = typename std::enable_if<T, Immutable_opensim_mesh_instance>::type>
        typename std::enable_if<T, Immutable_opensim_mesh_instance>::type operator*() noexcept {
            return Immutable_opensim_mesh_instance{geom.associated_components[pos], geom.drawlist.instances[pos]};
        }

        template<bool T = !IsConst, typename = typename std::enable_if<T, Mutable_opensim_mesh_instance>::type>
        typename std::enable_if<T, Mutable_opensim_mesh_instance>::type operator*() noexcept {
            return Mutable_opensim_mesh_instance{geom.associated_components[pos], geom.drawlist.instances[pos]};
        }

        bool operator!=(OpenSim_geometry_iterator const& other) const noexcept {
            return pos != other.pos;
        }

        OpenSim_geometry_iterator& operator++() noexcept {
            ++pos;
            return *this;
        }
    };

    inline osmv::OpenSim_geometry_iterator<true> begin(osmv::OpenSim_model_geometry const& geom) {
        return osmv::OpenSim_geometry_iterator<true>{const_cast<osmv::OpenSim_model_geometry&>(geom), 0};
    }

    inline osmv::OpenSim_geometry_iterator<true> cbegin(osmv::OpenSim_model_geometry const& geom) {
        return osmv::OpenSim_geometry_iterator<true>{const_cast<osmv::OpenSim_model_geometry&>(geom), 0};
    }

    inline osmv::OpenSim_geometry_iterator<false> begin(osmv::OpenSim_model_geometry& geom) {
        return osmv::OpenSim_geometry_iterator<false>{geom, 0};
    }

    inline osmv::OpenSim_geometry_iterator<true> end(osmv::OpenSim_model_geometry const& geom) {
        return osmv::OpenSim_geometry_iterator<true>{const_cast<osmv::OpenSim_model_geometry&>(geom),
                                                     geom.associated_components.size()};
    }

    inline osmv::OpenSim_geometry_iterator<true> cend(osmv::OpenSim_model_geometry const& geom) {
        return osmv::OpenSim_geometry_iterator<true>{const_cast<osmv::OpenSim_model_geometry&>(geom),
                                                     geom.associated_components.size()};
    }

    inline osmv::OpenSim_geometry_iterator<false> end(osmv::OpenSim_model_geometry& geom) {
        return osmv::OpenSim_geometry_iterator<false>{const_cast<osmv::OpenSim_model_geometry&>(geom),
                                                      geom.associated_components.size()};
    }
}

namespace osmv {

    // runtime rendering flags: the renderer uses these to make rendering decisions
    using SimpleModelRendererFlags = int;
    enum SimpleModelRendererFlags_ {
        SimpleModelRendererFlags_None = 0,

        // camera is in a "currently dragging" state
        SimpleModelRendererFlags_Dragging = 1 << 0,

        // camera is in a "currently panning" state
        SimpleModelRendererFlags_Panning = 1 << 1,

        // renderer should draw in wireframe mode
        SimpleModelRendererFlags_WireframeMode = 1 << 2,

        // renderer should draw mesh normals
        SimpleModelRendererFlags_ShowMeshNormals = 1 << 3,

        // renderer should draw a chequered floor
        SimpleModelRendererFlags_ShowFloor = 1 << 4,

        // renderer should draw selection rims
        SimpleModelRendererFlags_DrawRims = 1 << 5,

        // renderer should draw dynamic OpenSim model decorations
        SimpleModelRendererFlags_DrawDynamicDecorations = 1 << 6,

        // renderer should draw static OpenSim model decorations
        SimpleModelRendererFlags_DrawStaticDecorations = 1 << 7,

        // perform hover testing on dynamic decorations
        SimpleModelRendererFlags_HoverableDynamicDecorations = 1 << 8,

        // perform hover testing on static decorations
        SimpleModelRendererFlags_HoverableStaticDecorations = 1 << 9,

        SimpleModelRendererFlags_Default = SimpleModelRendererFlags_ShowFloor | SimpleModelRendererFlags_DrawRims |
                                           SimpleModelRendererFlags_DrawDynamicDecorations |
                                           SimpleModelRendererFlags_DrawStaticDecorations |
                                           SimpleModelRendererFlags_HoverableDynamicDecorations
    };

    // a renderer that draws an OpenSim::Model + SimTK::State pair into the current
    // framebuffer using a basic polar camera that can swivel around the model
    struct Simple_model_renderer final {
    private:
        Raw_renderer renderer;

    public:
        // this is set whenever the implementation detects that the mouse is over
        // a component (provided hover detection is enabled in the flags)
        int hovertest_x = -1;
        int hovertest_y = -1;
        OpenSim::Component const* hovered_component = nullptr;

        // not currently runtime-editable
        static constexpr float fov = 120.0f;
        static constexpr float znear = 0.1f;
        static constexpr float zfar = 100.0f;
        static constexpr float mouse_wheel_sensitivity = 0.9f;
        static constexpr float mouse_drag_sensitivity = 1.0f;

        float radius = 5.0f;
        float theta = 0.88f;
        float phi = 0.4f;
        glm::vec3 pan = {0.3f, -0.5f, 0.0f};
        glm::vec3 light_pos = {1.5f, 3.0f, 0.0f};
        glm::vec3 light_rgb = {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f};
        glm::vec4 background_rgba = {0.89f, 0.89f, 0.89f, 1.0f};
        glm::vec4 rim_rgba = {1.0f, 0.4f, 0.0f, 0.85f};
        float rim_thickness = 0.00075f;

        SimpleModelRendererFlags flags = SimpleModelRendererFlags_Default;

        // populated by calling generate_geometry(Model, State)
        OpenSim_model_geometry geometry;

    public:
        Simple_model_renderer(int w, int h, int samples);
        Simple_model_renderer(Simple_model_renderer const&) = delete;
        Simple_model_renderer(Simple_model_renderer&&) = delete;
        Simple_model_renderer& operator=(Simple_model_renderer const&) = delete;
        Simple_model_renderer& operator=(Simple_model_renderer&&) = delete;
        ~Simple_model_renderer() noexcept;

        void reallocate_buffers(int w, int h, int samples);

        // handle event (probably forwarded from a screen)
        //
        // returns `true` if the event was handled, `false` otherwise
        bool on_event(SDL_Event const&);

        // populate `this->geometry` with geometry from the model + state pair, but don't
        // draw it on the screen
        //
        // this (advanced) approach is here so that callers can modify the draw list
        // before drawing (e.g. to custom-color components)
        void generate_geometry(OpenSim::Model const&, SimTK::State const&);

        // apply rim colors for selected/hovered components in `this->geometry`
        //
        // note: you don't *need* to call this: it's a convenience method for the most common
        //       use-case of having selected and hovered components in the scene
        void apply_standard_rim_coloring(OpenSim::Component const* selected = nullptr);

        // draw `this->geometry`
        //
        // note: this assumes you previously called `generate_geometry`
        // note #2: `draw`ing mutates `this->geometry`
        gl::Texture_2d& draw();

        // an "on rails" draw call that utilizes the more advanced API
        //
        // use this until you need to customize things
        gl::Texture_2d&
            draw(OpenSim::Model const& model, SimTK::State const& st, OpenSim::Component const* selected = nullptr) {
            generate_geometry(model, st);
            apply_standard_rim_coloring(selected);
            return draw();
        }
    };
}

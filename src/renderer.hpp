#pragma once

#include "screen.hpp"
#include "sdl_wrapper.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <memory>
#include <vector>

namespace SimTK {
    class State;
}

namespace OpenSim {
    class Model;
    class Component;
}

namespace osmv {
    class Application;

    // one instance of a mesh extracted from an OpenSim model
    struct Mesh_instance final {
        // The component from which the mesh instance was generated
        OpenSim::Component const* owner;

        // transforms mesh vertices into scene worldspace
        glm::mat4 transform;

        // normal transform: transforms mesh normals into scene worldspace
        //
        // this is mostly here as a draw-time optimization because it's expensive
        // to compute. If you're editing `transform` (above), then you *may* need
        // to update this also. The easiest way is:
        //
        //     glm::transpose(glm::inverse(transform));
        glm::mat4 normal_xform;

        // mesh RGBA color
        //
        // note: alpha blending is expensive. Most mesh instances should keep
        // A == 1.0f
        glm::vec4 rgba;

        // INTERNAL: alpha strength of rim highlights [0.0, 1.0f]
        float _rim_alpha = 0.0f;

        // INTERNAL: ID for the mesh instance's vertices (e.g. sphere, skull) that the
        // renderer should render
        //
        // don't play with this unless you know what you're doing: it's an internal
        // field that the renderer uses when computing a draw call
        int _meshid;

        Mesh_instance(
            OpenSim::Component const* _owner, glm::mat4 const& _transform, glm::vec4 const& _rgba, int __meshid) :
            owner{_owner},
            transform{_transform},
            normal_xform{glm::transpose(glm::inverse(transform))},
            rgba{_rgba},
            _meshid{__meshid} {
        }

        // set the strength of the rim higlights for the element
        //
        // set this != 0.0 if you want a rim around the outside of this particular
        // mesh instance. Useful for selection highlighting, etc.
        //
        // This will be set to 0.0 for any freshly-generated `Mesh_instance`s
        void set_rim_strength(float strength) {
            _rim_alpha = strength;
        }
    };

    // all geometry pulled out of one state of an OpenSim::Model
    struct State_geometry final {
        std::vector<Mesh_instance> meshes;

        void clear() {
            meshes.clear();
        }
    };

    // flags for the geometry generator
    using GeometryGeneratorFlags = int;
    enum GeometryGeneratorFlags_ {
        // here for completeness
        GeometryGeneratorFlags_None = 0,

        // only generate geometry for static decorations in the model
        GeometryGeneratorFlags_Static = 1 << 0,

        // only generate geometry for dynamic decorations in the model
        GeometryGeneratorFlags_Dynamic = 1 << 1,

        // default flags
        GeometryGeneratorFlags_Default = GeometryGeneratorFlags_Static | GeometryGeneratorFlags_Dynamic
    };

    struct Renderer_impl;
    struct Renderer final {
        // camera parameters
        //
        // perspective camera using polar coordinates for spinning around the model
        float radius = 5.0f;
        float theta = 0.88f;
        float phi = 0.4f;
        glm::vec3 pan = {0.3f, -0.5f, 0.0f};
        float fov = 120.0f;
        float znear = 0.1f;
        float zfar = 100.0f;

        // event parameters
        bool dragging = false;
        bool panning = false;
        float mouse_wheel_sensitivity = 0.9f;
        float mouse_drag_sensitivity = 1.0f;

        // rendering parameters
        glm::vec3 light_pos = {1.5f, 3.0f, 0.0f};
        glm::vec3 light_rgb = {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f};
        glm::vec4 background_rgba = {0.89f, 0.89f, 0.89f, 1.0f};
        bool wireframe_mode = false;
        bool show_mesh_normals = false;
        bool show_floor = true;

        // set if user's mouse is over a component with `owner` set in the Mesh_instance
        OpenSim::Component const* hovered_component = nullptr;

    private:
        std::unique_ptr<Renderer_impl> state;

    public:
        Renderer(Application const&);
        Renderer(Renderer const&) = delete;
        Renderer(Renderer&&) = delete;
        Renderer& operator=(Renderer const&) = delete;
        Renderer& operator=(Renderer&&) = delete;
        ~Renderer() noexcept;

        Event_response on_event(Application&, SDL_Event const&);

        // generate geometry for an OpenSim::Model in a particular SimTK::State and
        // write that geometry to the outparam
        void generate_geometry(
            OpenSim::Model const&,
            SimTK::State const&,
            State_geometry&,
            GeometryGeneratorFlags flags = GeometryGeneratorFlags_Default);

        // draw scene geometry onto current framebuffer
        //
        // note: the renderer *may* reorder (but not mutate) the geometry contained in
        //       State_geometry. It does this for various technical technical reasons
        //       (notably: that rendering might require meshes to be drawn in a certain
        //       order)
        void draw(Application const&, State_geometry&);

        // this is the "easy mode" way of drawing a model state onto the
        // current framebuffer
        void draw(
            Application const& app,
            OpenSim::Model const& model,
            SimTK::State const& st,
            OpenSim::Component const* selected = nullptr) {

            static State_geometry geom;
            geom.clear();

            generate_geometry(model, st, geom);

            for (Mesh_instance& mi : geom.meshes) {
                if (selected != nullptr and selected == mi.owner) {
                    mi.set_rim_strength(1.0f);
                } else if (hovered_component != nullptr and hovered_component == mi.owner) {
                    mi.set_rim_strength(0.2f);
                }
            }

            draw(app, geom);
        }
    };
}

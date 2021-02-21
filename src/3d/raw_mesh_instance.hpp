#pragma once

#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <cstddef>
#include <utility>

namespace osmv {
    struct Rgba32 final {
        unsigned char r;
        unsigned char g;
        unsigned char b;
        unsigned char a;

        Rgba32() = default;

        Rgba32(glm::vec4 const& v) :
            r{static_cast<unsigned char>(255.0f * v.r)},
            g{static_cast<unsigned char>(255.0f * v.g)},
            b{static_cast<unsigned char>(255.0f * v.b)},
            a{static_cast<unsigned char>(255.0f * v.a)} {
        }
    };

    struct Passthrough_data final {
        unsigned char b0;
        unsigned char b1;

        static constexpr Passthrough_data from_u16(uint16_t v) noexcept {
            unsigned char b0 = v & 0xff;
            unsigned char b1 = (v >> 8) & 0xff;
            return Passthrough_data{b0, b1};
        }

        constexpr uint16_t to_u16() const noexcept {
            uint16_t rv = b0;
            rv |= static_cast<uint16_t>(b1) << 8;
            return rv;
        }
    };

    template<typename Mtx>
    static glm::mat3 normal_matrix(Mtx&& m) {
        glm::mat3 top_left{m};
        return glm::inverse(glm::transpose(top_left));
    }

    // one instance of a mesh
    //
    // this struct is fairly complicated because it has to pack data together ready for a
    // GPU draw call. Instanced GPU drawing requires that the data is contiguous and has all
    // necessary draw parameters (transform matrices, etc.) at predictable memory offsets.
    struct alignas(32) Raw_mesh_instance final {
        // transforms mesh vertices into scene worldspace
        glm::mat4x3 transform;

        // INTERNAL: normal transform: transforms mesh normals into scene worldspace
        //
        // this is mostly here as a draw-time optimization because it is redundant to compute
        // it every draw call (and because instanced rendering requires this to be available
        // in this struct)
        glm::mat3 _normal_xform;

        // primary mesh RGBA color
        //
        // this color is subject to mesh shading (lighting, shadows), so the rendered color may
        // differ
        //
        // note: alpha blending can be expensive. You should try to keep geometry opaque,
        //       unless you *really* need blending
        Rgba32 rgba;

        // INTERNAL: passthrough data
        //
        // this is used internally by the renderer to pass data between shaders, enabling
        // screen-space logic (selection logic, rim highlights, etc.)
        //
        // currently used for:
        //
        //     - r+g: raw passthrough data, used to handle selection logic. Downstream renderers
        //            use these channels to encode logical information (e.g. "an OpenSim component")
        //            into screen-space (e.g. "A pixel from an OpenSim component")
        //
        //     - b:   unused (reserved)
        //
        //     - a:   rim alpha. Used to calculate how strongly (if any) rims should be drawn
        //            around the rendered geometry. Used for highlighting elements in the scene
        Rgba32 _passthrough;

        // INTERNAL: mesh ID: globally unique ID for the mesh vertices that should be rendered
        //
        // the renderer uses this ID to deduplicate and instance draw calls. You shouldn't mess
        // with this unless you know what you're doing
        int _meshid;

        // trivial ctor: useful if the caller knows what they're doing and some STL
        //               algorithms like when a type is trivially constructable
        Raw_mesh_instance() = default;

        template<typename Mat4x3, typename Rgba>
        Raw_mesh_instance(Mat4x3&& _transform, Rgba&& _rgba, int meshid) noexcept :
            transform{std::forward<Mat4x3>(_transform)},
            _normal_xform{normal_matrix(transform)},
            rgba{std::forward<Rgba>(_rgba)},
            _passthrough{},
            _meshid{meshid} {
        }

        void set_rim_alpha(unsigned char a) noexcept {
            _passthrough.a = a;
        }

        // set passthrough data
        //
        // note: wherever the scene *isn't* rendered, black (0x000000) is encoded, so users of
        //       this should treat 0x000000 as "reserved"
        void set_passthrough_data(Passthrough_data pd) noexcept {
            _passthrough.r = pd.b0;
            _passthrough.g = pd.b1;
        }

        Passthrough_data passthrough_data() const noexcept {
            return {_passthrough.r, _passthrough.g};
        }
    };
}

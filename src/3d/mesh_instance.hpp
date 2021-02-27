#pragma once

#include "src/3d/gpu_data_reference.hpp"

#include <glm/mat3x3.hpp>
#include <glm/mat4x3.hpp>
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

        constexpr Rgba32(glm::vec4 const& v) noexcept :
            r{static_cast<unsigned char>(255.0f * v.r)},
            g{static_cast<unsigned char>(255.0f * v.g)},
            b{static_cast<unsigned char>(255.0f * v.b)},
            a{static_cast<unsigned char>(255.0f * v.a)} {
        }
    };

    struct Rgb24 final {
        unsigned char r;
        unsigned char g;
        unsigned char b;
    };

    struct Passthrough_data final {
        unsigned char b0;
        unsigned char b1;

        [[nodiscard]] static constexpr Passthrough_data from_u16(uint16_t v) noexcept {
            unsigned char b0 = v & 0xff;
            unsigned char b1 = (v >> 8) & 0xff;
            return Passthrough_data{b0, b1};
        }

        [[nodiscard]] constexpr uint16_t to_u16() const noexcept {
            uint16_t rv = b0;
            rv |= static_cast<uint16_t>(b1) << 8;
            return rv;
        }
    };

    [[nodiscard]] inline constexpr bool operator<(Passthrough_data const& a, Passthrough_data const& b) noexcept {
        return a.to_u16() < b.to_u16();
    }

    template<typename Mtx>
    static constexpr glm::mat3 normal_matrix(Mtx&& m) noexcept {
        glm::mat3 top_left{m};
        return glm::inverse(glm::transpose(top_left));
    }

    // one instance of a mesh
    //
    // this struct is fairly complicated and densely packed because it is *exactly* what
    // will be copied to the GPU at runtime. Size + alignment can matter *a lot*.
    struct alignas(32) Mesh_instance final {
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
        //     - b:   rim alpha. Used to calculate how strongly (if any) rims should be drawn
        //            around the rendered geometry. Used for highlighting elements in the scene
        Rgb24 _passthrough;

        unsigned char pad = 0;

        // INTERNAL: mesh ID: globally unique ID for the mesh vertices that should be rendered
        //
        // the renderer uses this ID to deduplicate and instance draw calls. You shouldn't mess
        // with this unless you know what you're doing
        Mesh_reference _meshid;

        Texture_reference _diffuse_texture;

        template<typename Mat4x3, typename Rgba>
        constexpr Mesh_instance(Mat4x3&& _transform, Rgba&& _rgba, Mesh_reference meshid) noexcept :
            transform{std::forward<Mat4x3>(_transform)},
            _normal_xform{normal_matrix(transform)},
            rgba{std::forward<Rgba>(_rgba)},
            _passthrough{},
            _meshid{meshid},
            _diffuse_texture{Texture_reference::invalid()} {
        }

        template<typename Mat4x3, typename Rgba>
        constexpr Mesh_instance(
            Mat4x3&& _transform, Rgba&& _rgba, Mesh_reference mesh, Texture_reference tex) noexcept :
            transform{std::forward<Mat4x3>(_transform)},
            _normal_xform{normal_matrix(transform)},
            rgba{std::forward<Rgba>(_rgba)},
            _passthrough{},
            _meshid{mesh},
            _diffuse_texture{tex} {
        }

        constexpr void set_rim_alpha(unsigned char a) noexcept {
            _passthrough.b = a;
        }

        // set passthrough data
        //
        // note: wherever the scene *isn't* rendered, black (0x000000) is encoded, so users of
        //       this should treat 0x000000 as "reserved"
        constexpr void set_passthrough_data(Passthrough_data pd) noexcept {
            _passthrough.r = pd.b0;
            _passthrough.g = pd.b1;
        }

        [[nodiscard]] constexpr Passthrough_data passthrough_data() const noexcept {
            return {_passthrough.r, _passthrough.g};
        }
    };
}

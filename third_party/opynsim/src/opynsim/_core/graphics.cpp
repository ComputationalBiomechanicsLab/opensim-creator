#include "graphics.h"

#include <opynsim/_core/type_casters/vector.h>
#include <opynsim/_core/_core.h>

#include <libopynsim/graphics/render_model_in_state.h>
#include <liboscar/graphics/scene/scene_cache.h>
#include <libopynsim/model.h>
#include <libopynsim/model_state.h>
#include <liboscar/graphics/camera_v2.h>
#include <liboscar/graphics/mesh.h>
#include <liboscar/graphics/texture2d.h>
#include <liboscar/maths/geometric_functions.h>
#include <liboscar/utilities/assertions.h>
#include <nanobind/nanobind.h>
#pragma warning(push)
#pragma warning(disable : 4702) // Disable "unreachable code"
#include <nanobind/ndarray.h>
#pragma warning(pop)
#include <nanobind/stl/array.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/unique_ptr.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <new>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace nb = nanobind;
using namespace opyn;

namespace
{
    /// Wrap a unique pointer into a capsule.
    template<typename T>
    nb::capsule to_capsule(std::unique_ptr<T> p)
    {
        return nb::capsule{
            p.release(),
            [](void* ptr) noexcept { delete static_cast<T*>(ptr); }  // NOLINT(cppcoreguidelines-owning-memory)
        };
    }

    /// Vertically flips `data`, which is assumed to be in a `dimensions.x()` by `dimensions.y()` grid.
    template<typename T>
    void vertically_flip(std::span<T> data, osc::Vector2uz dimensions)
    {
        OSC_ASSERT(data.size() == osc::area_of(dimensions));
        for (size_t row = 0; row < dimensions.y() / 2; ++row) {
            const auto top_row_it = data.begin() + (row * dimensions.x());
            const auto bottom_row_it = data.begin() + ((dimensions.y() - 1 - row) * dimensions.x());
            std::swap_ranges(top_row_it, top_row_it + dimensions.x(), bottom_row_it);
        }
    }

    /// Returns `pixels`, which is assumed to be in a `dimensions.x()` by `dimensions.y()` grid, as
    /// a Python-compatible `ndarray`.
    template<typename T>
    auto to_ndarray(
        std::unique_ptr<std::vector<T>> pixels,
        osc::Vector2uz dimensions) -> nb::ndarray<nb::numpy, const uint8_t, nb::shape<-1, -1, std::tuple_size_v<T>>>
    {
        static_assert(sizeof(typename T::value_type) == 1);
        static_assert(sizeof(T) == std::tuple_size_v<T> * sizeof(typename T::value_type));

        const uint8_t* pixel_pointer = std::launder(reinterpret_cast<const uint8_t*>(pixels->data()));
        const auto shape = std::to_array<size_t>({ dimensions.y(), dimensions.x(), std::tuple_size_v<T> });

        return {pixel_pointer, shape.size(), shape.data(), to_capsule(std::move(pixels))};
    }

    nb::ndarray<nb::numpy, const uint8_t, nb::shape<-1, -1, 4>> pixels_rgba32_impl(
        const osc::Texture2D& texture_2d)
    {
        // Get the pixels from the texture
        const osc::Vector2uz pixel_dimensions = texture_2d.pixel_dimensions();
        auto decoded_pixels = std::make_unique<std::vector<osc::Color32>>(texture_2d.pixels32());

        // Vertically flip the pixels. This is necessary because oscar uses
        // a "Y points up" encoding whereas almost all Python libraries use
        // a "Y points down" encoding.
        vertically_flip(std::span{*decoded_pixels}, pixel_dimensions);

        // Convert into a 2-dimensional ndarray
        return to_ndarray(std::move(decoded_pixels), pixel_dimensions);
    }

    nb::ndarray<nb::numpy, const uint8_t, nb::shape<-1, -1, 3>> pixels_rgb24_impl(
        const osc::Texture2D& texture_2d)
    {
        // Get the pixels from the texture
        const osc::Vector2uz pixel_dimensions = texture_2d.pixel_dimensions();
        auto decoded_pixels = std::make_unique<std::vector<osc::Color24>>(texture_2d.pixels24());

        // Vertically flip the pixels. This is necessary because oscar uses
        // a "Y points up" encoding whereas almost all Python libraries use
        // a "Y points down" encoding.
        vertically_flip(std::span{*decoded_pixels}, pixel_dimensions);

        // Convert into a 2-dimensional ndarray
        return to_ndarray(std::move(decoded_pixels), pixel_dimensions);
    }

    nb::ndarray<nb::numpy, const float, nb::shape<-1, 3>> mesh_vertices_impl(
        const osc::Mesh& mesh)
    {
        static_assert(sizeof(osc::Vector3f) == 3*sizeof(float), "required to cast the pointer to float with no gaps between vertices");
        static_assert(alignof(osc::Vector3f) == alignof(float));

        // Copy the vertices out of the `Mesh` and into a capsule-friendly pointer
        auto vertices = std::make_unique<std::vector<osc::Vector3f>>(mesh.vertices());

        // Cast the front vertex pointer from `const Vector3f*` to `const float*`, so that
        // it matches what the ndarray API needs.
        const auto* data_ptr = std::launder(reinterpret_cast<const float*>(vertices->data()));
        constexpr size_t ndim = 2;
        const auto shape = std::to_array<size_t>({ vertices->size(), 3 });

        return {data_ptr, ndim, shape.data(), to_capsule(std::move(vertices))};
    }

    void mesh_set_vertices_impl(
        osc::Mesh& mesh,
        const nb::ndarray<nb::ro, nb::shape<-1, 3>, nb::c_contig, nb::device::cpu>& input)
    {
        const size_t num_vertices = input.shape(0);

        if (input.dtype() == nb::dtype<float>()) {
            // Fast path: the vertex data from the caller matches the float32 format
            // that's always presented via the getters, so just copy their data into
            // the mesh.

            static_assert(sizeof(osc::Vector3f) == 3*sizeof(float));
            static_assert(alignof(osc::Vector3f) <= alignof(float));
            mesh.set_vertices({static_cast<const osc::Vector3f*>(input.data()), num_vertices});

        } else if (input.dtype() == nb::dtype<double>()) {
            // Slow path: the vertex data from the caller is 64-bit, but the `Mesh` class
            // always returns 32-bit encoded vertices so, to keep things simple, the data
            // should be converted to 32-bit.

            static_assert(sizeof(osc::Vector3d) == 3*sizeof(double));
            static_assert(alignof(osc::Vector3d) <= alignof(double));
            const std::span<const osc::Vector3d> source_vertices{
                static_cast<const osc::Vector3d*>(input.data()),
                num_vertices
            };

            std::vector<osc::Vector3f> converted_vertices;
            converted_vertices.reserve(num_vertices);
            for (const auto& source_vertex : source_vertices) {
                converted_vertices.emplace_back(source_vertex);
            }
            mesh.set_vertices(converted_vertices);
        } else {
            throw nb::type_error("Unsupported array type! Property accepts float32 or float64.");
        }
    }

    nb::ndarray<nb::numpy, const int32_t, nb::shape<-1>> mesh_faces_impl(
        const osc::Mesh& mesh)
    {
        // Read the `Mesh`'s indices into a pointer that can be put into a capsule
        const auto indices_view = mesh.indices();
        auto faces = std::make_unique<std::vector<int32_t>>();
        faces->reserve(indices_view.size());
        for (const auto& index : indices_view) {
            faces->emplace_back(index);
        }

        const int32_t* faces_ptr = faces->data();
        const size_t size = faces->size();

        return {faces_ptr, {size}, to_capsule(std::move(faces))};
    }

    template<typename To, typename From>
    void mesh_assign_converted_indices(osc::Mesh& mesh, std::span<const From> from)
    {
        std::vector<To> data;
        data.reserve(from.size());
        for (const From& face : from) {
            data.emplace_back(static_cast<To>(face));
        }
        mesh.set_indices(data);
    }

    void mesh_set_faces_impl(
        osc::Mesh& mesh,
        const nb::ndarray<nb::ro, nb::shape<-1>, nb::c_contig, nb::device::cpu>& input)
    {
        const size_t num_faces = input.shape(0);

        if (input.dtype() == nb::dtype<int32_t>()) {
            const std::span<const int32_t> faces{static_cast<const int32_t*>(input.data()), num_faces};
            mesh_assign_converted_indices<uint32_t>(mesh, faces);
        } else if (input.dtype() == nb::dtype<int64_t>()) {
            const std::span<const int64_t> faces{static_cast<const int64_t*>(input.data()), num_faces};
            mesh_assign_converted_indices<uint32_t>(mesh, faces);
        } else {
            throw nb::type_error("Unsupported array type! The faces property only accepts int32/int64 data types.");
        }
    }

    void def_color(nanobind::module_& m)
    {
        using osc::Color;

        nb::class_<Color> cls(m, "Color", R"(
            Represents an RGBA color in an sRGB color space with normalized 32-bit floating-point components.
        )");
        static_assert(Color{} == Color::clear());
        cls.def(nb::init<>{});
        cls.def(nb::init<float, float, float, float>{}, nb::arg("r"), nb::arg("g"), nb::arg("b"), nb::arg("a") = 1.0f);
        cls.def_prop_ro_static("clear", [](nb::handle) { return Color::clear(); }, nb::rv_policy::copy);
        cls.def_prop_ro_static("white", [](nb::handle) { return Color::white(); }, nb::rv_policy::copy);
        cls.def_prop_ro_static("black", [](nb::handle) { return Color::black(); }, nb::rv_policy::copy);
        cls.def_prop_ro_static("red",   [](nb::handle) { return Color::red();   }, nb::rv_policy::copy);
        cls.def_prop_ro_static("green", [](nb::handle) { return Color::green(); }, nb::rv_policy::copy);
        cls.def_prop_ro_static("blue",  [](nb::handle) { return Color::blue();  }, nb::rv_policy::copy);
        cls.def("__eq__", std::equal_to<Color>{});
        cls.def("__hash__",  [](const Color& c) { return std::hash<Color>{}(c); });
        cls.def("__repr__",  [](const Color& c) { return std::format("{}", c); });
        cls.def_prop_rw("r", [](const Color& c) { return c.r; }, [](Color& self, float rhs) { self.r = rhs; });
        cls.def_prop_rw("g", [](const Color& c) { return c.g; }, [](Color& self, float rhs) { self.g = rhs; });
        cls.def_prop_rw("b", [](const Color& c) { return c.b; }, [](Color& self, float rhs) { self.b = rhs; });
        cls.def_prop_rw("a", [](const Color& c) { return c.a; }, [](Color& self, float rhs) { self.a = rhs; });
    }

    void def_camera(nanobind::module_& m) {
        nb::class_<osc::CameraV2> cls(m, "Camera", R"(
            Represents a virtual camera at `position` in world space, looking along
            a `direction` vector. The `up` vector specifies which direction corresponds
            to the 'top' of the rendered image.
        )");
        cls.def(nb::init<>{});
        cls.def_prop_rw(
            "position",
            &osc::CameraV2::position,
            &osc::CameraV2::set_position,
            "Get/set the position of the camera in world space."
        );
        cls.def_prop_rw(
            "direction",
            &osc::CameraV2::direction,
            &osc::CameraV2::set_direction,
            "Get/set the normalized direction the camera is looking toward."
        );
        cls.def_prop_rw(
            "up",
            &osc::CameraV2::up,
            &osc::CameraV2::set_up,
            "Get/set the normalized direction vector that corresponds to the 'top' of the rendered image"
        );
    }

    void def_texture2d(nanobind::module_& graphics_module)
    {
        nb::class_<osc::Texture2D> texture2d_class(
            graphics_module,
            "Texture2D",
            R"(
                Represents a two-dimensional image.
            )"
        );
        texture2d_class.def(
            "pixels_rgba32",
            pixels_rgba32_impl,
            nb::rv_policy::move,
            R"(
                Returns an ndarray with the shape ``(height, width, 4)``, where each element of
                the 3rd dimension is a raw uint8 (0-255) pixel value for the red (R), green (G),
                blue (B), and alpha (A) components of the texture respectively.

                The layout of the pixels is such that the first pixel in the first row (``rv[0, 0]``)
                starts in the top-left of the image. Subsequent pixels within that row are then
                encoded left-to-right. The following row (``rv[1]``) is then below that row, and so on,
                until the final pixel, which is in the bottom-right of the image.

                The encoding of the components of a pixel is such that the lowest value for each
                component is zero, representing zero intensity of that component. The highest value
                for each component is 255, representing the maximum intensity of that
                component. The color components (R, G, and B) are in an sRGB color space, while the
                alpha component (A) is linear.
            )"
        );
        texture2d_class.def(
            "pixels_rgb24",
            pixels_rgb24_impl,
            nb::rv_policy::move,
            R"(
                Returns an ndarray with the shape ``(height, width, 3)``, where each element of
                the 3rd dimension is a raw uint8 (0-255) pixel value for the red (R), green (G),
                and blue (B) components of the texture respectively.

                The layout of the pixels is such that the first pixel in the first row (``rv[0, 0]``)
                starts in the top-left of the image. Subsequent pixels within that row are then
                encoded left-to-right. The following row (``rv[1]``) is then below that row, and so on,
                until the final pixel, which is in the bottom-right of the image.

                The encoding of the components of a pixel is such that the lowest value for each
                component is zero, representing zero intensity of that component. The highest value
                for each component is 255, representing the maximum intensity of that
                component. All components are in an sRGB color space.
            )"
        );
    }

    void def_mesh(nanobind::module_& graphics_module)
    {
        nb::class_<osc::Mesh> mesh_class(
            graphics_module,
            "Mesh",
            R"(
                Represents a renderable mesh.
            )"
        );
        mesh_class.def(nb::init<>());
        mesh_class.def_prop_rw(
            "vertices",
            mesh_vertices_impl,
            mesh_set_vertices_impl,
            nb::rv_policy::move,
            R"(
                Gets/sets the vertices of the mesh as an Nx3 ndarray.
            )"
        );
        mesh_class.def_prop_rw(
            "faces",
            mesh_faces_impl,
            mesh_set_faces_impl,
            nb::rv_policy::move,
            R"(
                Gets/sets the faces (indices) of the mesh as an 1-dimensional ndarray.
            )"
        );
    }

    void def_scene_cache(nanobind::module_& m)
    {
        nb::class_<osc::SceneCache> cls(
            m,
            "SceneCache",
            R"(
                Caches slow-to-load assets (shaders, meshes).
            )"
        );
        cls.def(nb::init<>{});
    }

    void def_render_model_in_state(nanobind::module_& m)
    {
        m.def(
            "render_model_in_state",
            [](const Model& model,
               const ModelState& model_state,
               std::pair<int, int> dimensions,
               const osc::Color& background_color,
               bool draw_floor,
               osc::SceneCache* scene_cache,
               const osc::CameraV2* camera)
            {
                return render_model_in_state(
                    get_lazy_loaded_opynsim_app(),
                    model,
                    model_state,
                    osc::Vector2{dimensions.first, dimensions.second},
                    background_color,
                    draw_floor,
                    scene_cache,
                    camera
                );
            },
            nb::arg("model"),
            nb::arg("model_state"),
            nb::kw_only{},
            nb::arg("dimensions") = std::make_pair(640, 480),
            nb::arg("background_color") = osc::Color::clear(),
            nb::arg("draw_floor") = false,
            nb::arg("scene_cache") = nullptr,
            nb::arg("camera") = std::nullopt,
            R"(
                Renders the given :class:`opynsim.Model` + :class:`opynsim.ModelState` to
                a :class:`opynsim.graphics.Texture2D`.

                Args:
                    model (opynsim.Model): The model to render.
                    model_state (opynsim.ModelState): The state of the model to render. Should be realized to at least :attr:`opynsim.ModelStateStage.REPORT`.
                    dimensions (tuple[int, int]): The desired output resolution (width, height) of the rendered image in pixels.
                    background_color (opynsim.graphics.Color): The desired background color of the rendered scene.
                    draw_floor (bool): Toggles drawing a chequered floor at Y=0 in the scene.
                    scene_cache (opynsim.graphics.SceneCache): A scene cache from which the implementation pulls cached scene elements (shaders, meshes, etc.). Otherwise, the implementation loads all assets every time this function is called (slow).
                    camera (opynsim.graphics.Camera): The camera to use when rendering (default: looks down -Z and pulls back from the origin until the entire model fits in-frame).

                Returns:
                    opynsim.graphics.Texture2D: The rendered image, which will have the specified ``dimensions``.
            )"
        );
    }
}

void opyn::init_graphics_submodule(nanobind::module_& graphics_module)
{
    def_color(graphics_module);
    def_scene_cache(graphics_module);
    def_texture2d(graphics_module);
    def_mesh(graphics_module);
    def_camera(graphics_module);
    def_render_model_in_state(graphics_module);
}

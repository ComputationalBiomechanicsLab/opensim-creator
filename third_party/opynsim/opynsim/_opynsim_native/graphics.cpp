#include "graphics.h"

#include <libopynsim/graphics/render_model_in_state.h>
#include <libopynsim/model.h>
#include <libopynsim/model_state.h>
#include <liboscar/graphics/texture2d.h>
#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/unique_ptr.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <new>
#include <utility>

namespace nb = nanobind;

namespace
{
    nb::ndarray<nb::numpy, const uint8_t, nb::shape<-1, -1, 4>> pixels_rgba32_impl(
        const osc::Texture2D& texture_2d)
    {
        static_assert(sizeof(osc::Color32) == 4, "required to cast the pointer to uint8_t with no gaps between pixels");
        static_assert(alignof(osc::Color32) == 4);
        static_assert(offsetof(osc::Color32, r) == 0);
        static_assert(offsetof(osc::Color32, g) == 1);
        static_assert(offsetof(osc::Color32, b) == 2);
        static_assert(offsetof(osc::Color32, a) == 3);

        // Get the pixels from the texture
        const osc::Vector2uz pixel_dimensions = texture_2d.pixel_dimensions();
        auto decoded_pixels = std::make_unique<std::vector<osc::Color32>>(texture_2d.pixels32());

        // Vertically flip the pixels. This is necessary because oscar uses
        // a "Y points up" encoding whereas almost all Python libraries use
        // a "Y points down" encoding.
        for (size_t row = 0; row < pixel_dimensions.y() / 2; ++row) {
            const auto top_row_it = decoded_pixels->begin() + (row * pixel_dimensions.x());
            const auto bottom_row_it = decoded_pixels->begin() + ((pixel_dimensions.y() - 1 - row) * pixel_dimensions.x());
            std::swap_ranges(top_row_it, top_row_it + pixel_dimensions.x(), bottom_row_it);
        }

        // Cast the pixel pointer from `const Color32*` to `const uint8_t*`, so that the
        // shape of the ndarray matches how Python APIs typically expect it (`(h, w, 4)`).
        const uint8_t* pixel_pointer = std::launder(reinterpret_cast<const uint8_t*>(decoded_pixels->data()));

        // Stuff the pixel data into a Python-compatible capsule so that its lifetime is controlled by Python
        nb::capsule buffer_capsule{
            decoded_pixels.release(),
            [](void* p) noexcept { delete static_cast<std::vector<osc::Color32>*>(p); }
        };

        const auto shape = std::to_array<size_t>({ pixel_dimensions.y(), pixel_dimensions.x(), 4 });
        return {pixel_pointer, 3, shape.data(), buffer_capsule};
    }
}

void opyn::init_graphics_submodule(nanobind::module_& graphics_module)
{
    graphics_module.def(
        "render_model_in_state",
        render_model_in_state,
        nb::arg("model"),
        nb::arg("state"),
        nb::arg("dimensions") = std::make_pair(640, 480),
        nb::arg("zoom_to_fit") = true,
        R"(
            Renders the given :class:`opynsim.Model` + :class:`opynsim.ModelState` to
            a :class:`opynsim.graphics.Texture2D`.

            Args:
                model (opynsim.Model): The model to render.
                state (opynsim.ModelState): The state of the model to render.
                dimensions (tuple[int, int]): The desired output resolution (width, height) of the rendered image in pixels. Defaults to (640, 480).
                zoom_to_fit (bool): Tells the renderer to automatically set up the camera to focus on the center of the bounds of the scene at a distance that can see the entire scene.

            Returns:
                opynsim.graphics.Texture2D: The rendered image, which will have the specified ``dimensions``.
        )"
    );

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
        R"(
            Returns an ndarray with the shape ``(height, width, 4)`` where each element of
            the 3rd dimension is a raw uint8 (0-255) pixel value for the red (R), green (G),
            blue (B), and alpha (A) components of the texture respectively.

            The grid layout and component encoding of the returned array tries to match the
            conventions used by other Python libraries (e.g. matplotlib, pyvista) and
            image tools so that the render is easy to compose into other systems.

            The layout of the pixels is such that the first pixel in the first row (``rv[0, 0]``)
            starts in the top-left of the image. Subsequent pixels within that row are then
            encoded left-to-right. The following row (``rv[1]``) is then below that row, and so on,
            until the final pixel, which is in the bottom-right of the image.

            The encoding the the components of a pixel is such that the lowest value for each
            component is zero, representing zero intensity of that component. The highest value
            for each component is 255, representing the maximum intensity of that
            component. The color components (R, G, and B) are in an sRGB color space, while the
            alpha component (A) is linear.
        )"
    );
}

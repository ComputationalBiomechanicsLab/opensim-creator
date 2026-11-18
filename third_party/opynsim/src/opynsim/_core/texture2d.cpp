#include "texture2d.h"

#include <opynsim/_core/utilities/nb_helpers.h>

#include <liboscar/graphics/color32.h>
#include <liboscar/graphics/texture2d.h>
#include <liboscar/maths/geometric_functions.h>
#include <liboscar/maths/vector.h>
#include <liboscar/utilities/assertions.h>
#include <nanobind/nanobind.h>
#pragma warning(push)
#pragma warning(disable : 4702)  // Disable "unreachable code"
#include <nanobind/ndarray.h>
#pragma warning(pop)

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstddef>
#include <memory>
#include <new>
#include <span>
#include <tuple>
#include <utility>
#include <vector>

namespace nb = nanobind;
using namespace opyn;

namespace
{
    /// Vertically flips `data`, which is assumed to be in a `dimensions.x()` by `dimensions.y()`
    /// grid.
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
    auto to_ndarray(std::unique_ptr<std::vector<T>> pixels, osc::Vector2uz dimensions)
        -> nb::ndarray<nb::numpy, const uint8_t, nb::shape<-1, -1, std::tuple_size_v<T>>>
    {
        static_assert(sizeof(typename T::value_type) == 1);
        static_assert(sizeof(T) == std::tuple_size_v<T> * sizeof(typename T::value_type));

        const uint8_t* pixel_pointer =
            std::launder(reinterpret_cast<const uint8_t*>(pixels->data()));
        const auto shape =
            std::to_array<size_t>({dimensions.y(), dimensions.x(), std::tuple_size_v<T>});

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
}

void opyn::python::def_texture2d(nanobind::module_& m)
{
    nb::class_<osc::Texture2D> texture2d_class(
        m,
        "Texture2D",
        R"(
            Represents a two-dimensional image.
        )");
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
        )");
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
        )");
}

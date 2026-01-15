#include "image.h"

#include <liboscar/graphics/ColorSpace.h>
#include <liboscar/graphics/Texture2D.h>
#include <liboscar/maths/CommonFunctions.h>
#include <liboscar/maths/GeometricFunctions.h>
#include <liboscar/platform/resource_stream.h>
#include <liboscar/tests/testoscarconfig.h>
#include <liboscar/utils/NullOStream.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <filesystem>

using namespace osc;

TEST(Image, read_into_texture_respects_sRGB_color_space)
{
    const auto path = std::filesystem::path{OSC_TEST_RESOURCES_DIR} / "awesomeface.png";
    const Texture2D loaded_texture = Image::read_into_texture(ResourceStream{path}, ColorSpace::sRGB);

    ASSERT_EQ(loaded_texture.color_space(), ColorSpace::sRGB);
}

TEST(Image, read_into_texture_respects_linear_color_space)
{
    const auto path = std::filesystem::path{OSC_TEST_RESOURCES_DIR} / "awesomeface.png";
    const Texture2D loaded_texture = Image::read_into_texture(ResourceStream{path}, ColorSpace::Linear);

    ASSERT_EQ(loaded_texture.color_space(), ColorSpace::Linear);
}

TEST(Image, read_into_texture_is_compatible_with_PNG_write)
{
    const auto path = std::filesystem::path{OSC_TEST_RESOURCES_DIR} / "awesomeface.png";
    const Texture2D loaded_texture = Image::read_into_texture(ResourceStream{path}, ColorSpace::Linear);

    NullOStream out;
    ASSERT_NO_THROW({ PNG::write(out, loaded_texture); });
    ASSERT_TRUE(out.was_written_to());
}

TEST(Image, read_into_texture_can_load_image_from_ResourceStream)
{
    const auto path = std::filesystem::path{OSC_TEST_RESOURCES_DIR} / "awesomeface.png";
    const Texture2D loaded_texture = Image::read_into_texture(
        ResourceStream{path},
        ColorSpace::sRGB
    );

    ASSERT_EQ(loaded_texture.pixel_dimensions(), Vector2i(512, 512));
}

TEST(Image, read_into_texture_throws_when_called_with_an_invalid_path)
{
    ASSERT_ANY_THROW(
    {
        Image::read_into_texture(
            ResourceStream{"oscar/textures/doesnt_exist.png"},
            ColorSpace::sRGB
        );
    });
}

TEST(Image, read_into_texture_with_TreatComponentsAsSpatialVectors_negates_Y)
{
    // This is just a quick and dirty test to ensure that the implementation is
    // actually flipping the G component of the image if the caller indicates that
    // the pixels of the image are actually spatial vectors (e.g. as in normal
    // maps).

    const auto path = std::filesystem::path{OSC_TEST_RESOURCES_DIR} / "awesomeface.png";

    const Texture2D normally_loaded_texture = Image::read_into_texture(ResourceStream{path}, ColorSpace::sRGB);
    const Texture2D spatial_texture = Image::read_into_texture(ResourceStream{path}, ColorSpace::sRGB, ImageLoadingFlag::TreatComponentsAsSpatialVectors);
    const auto normal_pixels = normally_loaded_texture.pixels();
    const auto corrected_pixels = spatial_texture.pixels();

    ASSERT_EQ(normally_loaded_texture.pixel_dimensions(), spatial_texture.pixel_dimensions());
    ASSERT_EQ(normal_pixels.size(), corrected_pixels.size());
    for (size_t i = 0; i < normal_pixels.size(); ++i) {
        const Color normal_pixel = normal_pixels[i];
        const Color corrected_pixel = corrected_pixels[i];
        constexpr float abs_error = 1.0f/255.0f;  // to account for the image encoding being non-floating-point.
        ASSERT_EQ(normal_pixel.r, corrected_pixel.r);
        ASSERT_NEAR(normal_pixel.g, 1.0f - corrected_pixel.g, abs_error) << "no correction applied?";
        ASSERT_EQ(normal_pixel.b, corrected_pixel.b);
        ASSERT_EQ(normal_pixel.a, corrected_pixel.a);
    }
}

TEST(JPEG, write_is_compatible_with_reader)
{
    constexpr std::array<Color32, 4> pixels = {
        Color::red(),  Color::green(),
        Color::blue(), Color::white(),
    };

    Texture2D texture({2, 2});
    texture.set_pixels32(pixels);

    std::ostringstream written_stream;
    JPEG::write(written_stream, texture, 1.0f);
    const std::string data = written_stream.str();

    ASSERT_FALSE(data.empty());

    std::istringstream input_stream{data};
    const Texture2D parsed_texture = Image::read_into_texture(input_stream, "data.jpeg", ColorSpace::sRGB);

    ASSERT_EQ(parsed_texture.pixel_dimensions(), Vector2i(2, 2));
    const auto parsed_pixels = parsed_texture.pixels32();

    // Ensure the pixels are approximately equal to (lossy compression) the ones that were put in.
    ASSERT_EQ(parsed_pixels.size(), pixels.size());
    for (size_t i = 0; i < parsed_pixels.size(); ++i) {
        for (size_t component = 0; component < std::tuple_size_v<Color32>; ++component) {
            ASSERT_TRUE(equal_within_absdiff(parsed_pixels[i][component].normalized_value(), pixels[i][component].normalized_value(), 0.01f));
        }
    }
}

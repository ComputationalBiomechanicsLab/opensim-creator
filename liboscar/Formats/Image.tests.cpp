#include "Image.h"

#include <liboscar/Graphics/ColorSpace.h>
#include <liboscar/Graphics/Texture2D.h>
#include <liboscar/Maths/CommonFunctions.h>
#include <liboscar/Platform/ResourceStream.h>
#include <liboscar/testing/testoscarconfig.h>
#include <liboscar/Utils/NullOStream.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <filesystem>

using namespace osc;
namespace rgs = std::ranges;

TEST(load_texture2D_from_image, respects_sRGB_color_space)
{
    const auto path = std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "awesomeface.png";
    const Texture2D loaded_texture = load_texture2D_from_image(ResourceStream{path}, ColorSpace::sRGB);

    ASSERT_EQ(loaded_texture.color_space(), ColorSpace::sRGB);
}

TEST(load_texture2D_from_image, respects_linear_color_space)
{
    const auto path = std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "awesomeface.png";
    const Texture2D loaded_texture = load_texture2D_from_image(ResourceStream{path}, ColorSpace::Linear);

    ASSERT_EQ(loaded_texture.color_space(), ColorSpace::Linear);
}

TEST(load_texture2D_from_image, is_compatible_with_write_to_png)
{
    const auto path = std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "awesomeface.png";
    const Texture2D loaded_texture = load_texture2D_from_image(ResourceStream{path}, ColorSpace::Linear);

    NullOStream out;
    ASSERT_NO_THROW({ write_to_png(loaded_texture, out); });
    ASSERT_TRUE(out.was_written_to());
}

TEST(load_texture2D_from_image, can_load_image_from_ResourceStream)
{
    const auto path = std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "awesomeface.png";
    const Texture2D loaded_texture = load_texture2D_from_image(
        ResourceStream{path},
        ColorSpace::sRGB
    );

    ASSERT_EQ(loaded_texture.dimensions(), Vec2i(512, 512));
}

TEST(load_texture2D_from_image, throws_when_called_with_an_invalid_path)
{
    ASSERT_ANY_THROW(
    {
        load_texture2D_from_image(
            ResourceStream{"oscar/textures/doesnt_exist.png"},
            ColorSpace::sRGB
        );
    });
}

TEST(write_to_jpeg, is_compatible_with_reader)
{
    constexpr std::array<Color32, 4> pixels = {
        Color::red(),  Color::green(),
        Color::blue(), Color::white(),
    };

    Texture2D texture({2, 2});
    texture.set_pixels32(pixels);

    std::ostringstream written_stream;
    write_to_jpeg(texture, written_stream, 1.0f);
    const std::string data = written_stream.str();

    ASSERT_FALSE(data.empty());

    std::istringstream input_stream{data};
    const Texture2D parsed_texture = load_texture2D_from_image(input_stream, "data.jpeg", ColorSpace::sRGB, ImageLoadingFlag::FlipVertically);

    ASSERT_EQ(parsed_texture.dimensions(), Vec2i(2, 2));
    const auto parsed_pixels = parsed_texture.pixels32();

    // Ensure the pixels are approximately equal to (lossy compression) the ones that were put in.
    ASSERT_EQ(parsed_pixels.size(), pixels.size());
    for (size_t i = 0; i < parsed_pixels.size(); ++i) {
        for (size_t component = 0; component < std::tuple_size_v<Color32>; ++component) {
            ASSERT_TRUE(equal_within_absdiff(parsed_pixels[i][component].normalized_value(), pixels[i][component].normalized_value(), 0.01f));
        }
    }
}

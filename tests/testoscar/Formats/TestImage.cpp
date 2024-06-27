#include <oscar/Formats/Image.h>

#include <testoscar/testoscarconfig.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/ColorSpace.h>
#include <oscar/Graphics/Texture2D.h>
#include <oscar/Platform/ResourceStream.h>
#include <oscar/Utils/NullOStream.h>

#include <array>
#include <filesystem>

using namespace osc;

TEST(Image, LoadTexture2DFromImageRespectsSRGBColorSpace)
{
    const auto path = std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "awesomeface.png";
    const Texture2D loaded_texture = load_texture2D_from_image(ResourceStream{path}, ColorSpace::sRGB);

    ASSERT_EQ(loaded_texture.color_space(), ColorSpace::sRGB);
}

TEST(Image, LoadTexture2DFromImageRespectsLinearColorSpace)
{
    const auto path = std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "awesomeface.png";
    const Texture2D loaded_texture = load_texture2D_from_image(ResourceStream{path}, ColorSpace::Linear);

    ASSERT_EQ(loaded_texture.color_space(), ColorSpace::Linear);
}

TEST(Image, CanLoadAndThenWriteImage)
{
    const auto path = std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "awesomeface.png";
    const Texture2D loaded_texture = load_texture2D_from_image(ResourceStream{path}, ColorSpace::Linear);

    NullOStream out;
    ASSERT_NO_THROW({ write_to_png(loaded_texture, out); });
    ASSERT_TRUE(out.was_written_to());
}

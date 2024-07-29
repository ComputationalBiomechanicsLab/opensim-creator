#include <oscar/Graphics/Cubemap.h>

#include <gtest/gtest.h>
#include <oscar/Utils/EnumHelpers.h>

#include <array>
#include <cstdint>
#include <type_traits>
#include <utility>

using namespace osc;

TEST(Cubemap, can_construct_1x1_RGBA32_Cubemap)
{
    Cubemap cubemap{1, TextureFormat::RGBA32};
}

TEST(Cubemap, constructor_throws_if_given_zero_width)
{
    ASSERT_ANY_THROW({ Cubemap cubemap(0, TextureFormat::RGBA32); });
}

TEST(Cubemap, constructor_throws_if_given_negative_width)
{
    ASSERT_ANY_THROW({ Cubemap cubemap(-5, TextureFormat::RGBA32); });
}

TEST(Cubemap, can_copy_construct)
{
    const Cubemap source{1, TextureFormat::RGBA32};
    Cubemap copy = source;  // NOLINT(performance-unnecessary-copy-initialization)
}

static_assert(std::is_nothrow_move_constructible_v<Cubemap>);

TEST(Cubemap, can_move_construct)
{
    Cubemap source{1, TextureFormat::RGBA32};
    Cubemap copy{std::move(source)};
}

TEST(Cubemap, can_copy_assign)
{
    const Cubemap first{1, TextureFormat::RGBA32};
    Cubemap second{2, TextureFormat::RGB24};
    second = first;

    ASSERT_EQ(second.width(), first.width());
    ASSERT_EQ(second.texture_format(), first.texture_format());
}

TEST(Cubemap, is_nothrow_assignable)
{
    static_assert(std::is_nothrow_assignable_v<Cubemap, Cubemap&&>);
}

TEST(Cubemap, can_move_assign)
{
    const int32_t first_width = 1;
    const TextureFormat first_format = TextureFormat::RGB24;
    Cubemap first{first_width, first_format};

    const int32_t second_width = 2;
    const TextureFormat second_format = TextureFormat::RGBA32;
    Cubemap second{second_width, second_format};
    second = std::move(first);

    ASSERT_NE(first_width, second_width);
    ASSERT_NE(first_format, second_format);
    ASSERT_EQ(second.width(), first_width);
    ASSERT_EQ(second.texture_format(), first_format);
}

TEST(Cubemap, is_nothrow_destructible)
{
    static_assert(std::is_nothrow_destructible_v<Cubemap>);
}
TEST(Cubemap, operator_equals_is_available)
{
    const Cubemap cubemap{1, TextureFormat::RGBA32};

    ASSERT_EQ(cubemap, cubemap);
}

TEST(Cubemap, operator_equals_returns_true_for_copies)
{
    const Cubemap cubemap{1, TextureFormat::RGBA32};
    const Cubemap copy = cubemap; // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_EQ(cubemap, copy);
}

TEST(Cubemap, operator_equals_returns_false_after_mutating_a_copy)
{
    const Cubemap cubemap{1, TextureFormat::RGBA32};

    Cubemap copy = cubemap;
    const std::array<uint8_t, 4> data{};
    copy.set_pixel_data(CubemapFace::PositiveX, data);

    ASSERT_NE(cubemap, copy);
}

TEST(Cubemap, operator_equals_is_reference_based_not_value_based)
{
    // landmine test: this just verifies that equality is
    // really just reference equality, rather than actual
    // value equality (which is better)
    //
    // if the implementation of Cubemap has been updated
    // to enabled value-equality (e.g. by comparing the actual
    // image invalid_data or using a strong hashing technique) then
    // this test can be deleted
    const Cubemap a{1, TextureFormat::RGBA32};
    const Cubemap b{1, TextureFormat::RGBA32};

    ASSERT_NE(a, b);
}

TEST(Cubemap, width_returns_width_provided_via_constructor)
{
    const int32_t width = 4;
    const Cubemap cubemap{width, TextureFormat::RGBA32};

    ASSERT_EQ(cubemap.width(), width);
}

TEST(Cubemap, format_returns_TextureFormat_provided_via_constructor)
{
    const TextureFormat format = TextureFormat::RGB24;
    const Cubemap cubemap{1, format};

    ASSERT_EQ(cubemap.texture_format(), format);
}

TEST(Cubemap, set_pixel_data_works_for_any_face_when_given_the_correct_number_of_pixel_bytes)
{
    const TextureFormat format = TextureFormat::RGBA32;
    const size_t bytes_per_pixel_for_format = num_bytes_per_pixel_in(format);
    const size_t width = 5;
    const size_t num_bytes_per_face = width*width*bytes_per_pixel_for_format;
    const std::vector<uint8_t> data(num_bytes_per_face);

    Cubemap cubemap{width, format};
    for (CubemapFace face : make_option_iterable<CubemapFace>()) {
        cubemap.set_pixel_data(face, data);
    }
}

TEST(Cubemap, set_pixel_data_throws_exception_if_given_invalid_number_of_bytes_for_RGBA32)
{
    const TextureFormat format = TextureFormat::RGBA32;
    const size_t invalid_num_bytes_per_pixel = 3;
    const size_t width = 5;
    const size_t invalid_num_bytes_per_face = width*width*invalid_num_bytes_per_pixel;
    const std::vector<uint8_t> invalid_data(invalid_num_bytes_per_face);

    Cubemap cubemap{width, format};
    for (CubemapFace face : make_option_iterable<CubemapFace>()) {
        ASSERT_ANY_THROW({ cubemap.set_pixel_data(face, invalid_data); });
    }
}

TEST(Cubemap, set_pixel_data_throws_if_given_invalid_number_of_bytes_for_RGB24)
{
    const TextureFormat format = TextureFormat::RGB24;
    const size_t invalid_num_bytes_per_pixel = 4;
    const size_t width = 5;
    const size_t invalid_num_bytes_per_face = width*width*invalid_num_bytes_per_pixel;
    const std::vector<uint8_t> invalid_data(invalid_num_bytes_per_face);

    Cubemap cubemap{width, format};
    for (CubemapFace face : make_option_iterable<CubemapFace>()) {
        ASSERT_ANY_THROW({ cubemap.set_pixel_data(face, invalid_data); });
    }
}

TEST(Cubemap, set_pixel_data_throws_if_given_invalid_number_of_bytes_for_its_width)
{
    const TextureFormat format = TextureFormat::RGBA32;
    const size_t bytes_per_pixel_for_format = num_bytes_per_pixel_in(format);
    const size_t width = 5;
    const size_t num_bytes = width*width*bytes_per_pixel_for_format;
    const size_t incorrect_num_bytes = num_bytes+3;
    const std::vector<uint8_t> invalid_data(incorrect_num_bytes);

    Cubemap cubemap{width, format};
    for (CubemapFace face : make_option_iterable<CubemapFace>()) {
        ASSERT_ANY_THROW({ cubemap.set_pixel_data(face, invalid_data); });
    }
}

TEST(Cubemap, set_pixel_data_works_with_floating_point_texture_format)
{
    const TextureFormat format = TextureFormat::RGBAFloat;
    const size_t bytes_per_pixel_for_format = num_bytes_per_pixel_in(format);
    const size_t width = 5;
    const size_t num_bytes_per_face = width*width*bytes_per_pixel_for_format;
    const std::vector<uint8_t> data(num_bytes_per_face);

    Cubemap cubemap{width, format};
    for (CubemapFace face : make_option_iterable<CubemapFace>()) {
        cubemap.set_pixel_data(face, data);
    }
}

TEST(Cubemap, wrap_mode_defaults_to_Repeat)
{
    ASSERT_EQ(Cubemap(1, TextureFormat::RGBA32).wrap_mode(), TextureWrapMode::Repeat);
}

TEST(Cubemap, set_wrap_mode_sets_wrap_mode)
{
    Cubemap cubemap{1, TextureFormat::RGBA32};
    ASSERT_EQ(cubemap.wrap_mode(), TextureWrapMode::Repeat);
    cubemap.set_wrap_mode(TextureWrapMode::Clamp);
    ASSERT_EQ(cubemap.wrap_mode(), TextureWrapMode::Clamp);
}

TEST(Cubemap, set_wrap_mode_sets_all_faces)
{
    Cubemap cubemap{1, TextureFormat::RGBA32};
    ASSERT_EQ(cubemap.wrap_mode(), TextureWrapMode::Repeat);
    ASSERT_EQ(cubemap.wrap_mode_u(), TextureWrapMode::Repeat);
    ASSERT_EQ(cubemap.wrap_mode_v(), TextureWrapMode::Repeat);
    ASSERT_EQ(cubemap.wrap_mode_w(), TextureWrapMode::Repeat);
    cubemap.set_wrap_mode(TextureWrapMode::Clamp);
    ASSERT_EQ(cubemap.wrap_mode(), TextureWrapMode::Clamp);
    ASSERT_EQ(cubemap.wrap_mode_u(), TextureWrapMode::Clamp);
    ASSERT_EQ(cubemap.wrap_mode_v(), TextureWrapMode::Clamp);
    ASSERT_EQ(cubemap.wrap_mode_w(), TextureWrapMode::Clamp);
}

TEST(Cubemap, set_wrap_mode_u_sets_U_axis_and_general_wrap_mode_getter)
{
    Cubemap cubemap{1, TextureFormat::RGBA32};
    ASSERT_EQ(cubemap.wrap_mode(), TextureWrapMode::Repeat);
    ASSERT_EQ(cubemap.wrap_mode_u(), TextureWrapMode::Repeat);
    ASSERT_EQ(cubemap.wrap_mode_v(), TextureWrapMode::Repeat);
    ASSERT_EQ(cubemap.wrap_mode_w(), TextureWrapMode::Repeat);
    cubemap.set_wrap_mode_u(TextureWrapMode::Clamp);
    ASSERT_EQ(cubemap.wrap_mode(), TextureWrapMode::Clamp);  // `wrap_mode()` is an alias for `wrap_mode_u()`
    ASSERT_EQ(cubemap.wrap_mode_u(), TextureWrapMode::Clamp);
    ASSERT_EQ(cubemap.wrap_mode_v(), TextureWrapMode::Repeat);
    ASSERT_EQ(cubemap.wrap_mode_w(), TextureWrapMode::Repeat);
}

TEST(Cubemap, set_wrap_mode_u_only_sets_U_axis)
{
    Cubemap cubemap{1, TextureFormat::RGBA32};
    ASSERT_EQ(cubemap.wrap_mode(), TextureWrapMode::Repeat);
    ASSERT_EQ(cubemap.wrap_mode_u(), TextureWrapMode::Repeat);
    ASSERT_EQ(cubemap.wrap_mode_v(), TextureWrapMode::Repeat);
    ASSERT_EQ(cubemap.wrap_mode_w(), TextureWrapMode::Repeat);
    cubemap.set_wrap_mode_v(TextureWrapMode::Clamp);
    ASSERT_EQ(cubemap.wrap_mode(), TextureWrapMode::Repeat);
    ASSERT_EQ(cubemap.wrap_mode_u(), TextureWrapMode::Repeat);
    ASSERT_EQ(cubemap.wrap_mode_v(), TextureWrapMode::Clamp);
    ASSERT_EQ(cubemap.wrap_mode_w(), TextureWrapMode::Repeat);
}

TEST(Cubemap, set_wrap_mode_w_only_sets_w_axis)
{
    Cubemap cubemap{1, TextureFormat::RGBA32};
    ASSERT_EQ(cubemap.wrap_mode(), TextureWrapMode::Repeat);
    ASSERT_EQ(cubemap.wrap_mode_u(), TextureWrapMode::Repeat);
    ASSERT_EQ(cubemap.wrap_mode_v(), TextureWrapMode::Repeat);
    ASSERT_EQ(cubemap.wrap_mode_w(), TextureWrapMode::Repeat);
    cubemap.set_wrap_mode_w(TextureWrapMode::Clamp);
    ASSERT_EQ(cubemap.wrap_mode(), TextureWrapMode::Repeat);
    ASSERT_EQ(cubemap.wrap_mode_u(), TextureWrapMode::Repeat);
    ASSERT_EQ(cubemap.wrap_mode_v(), TextureWrapMode::Repeat);
    ASSERT_EQ(cubemap.wrap_mode_w(), TextureWrapMode::Clamp);
}

TEST(Cubemap, filter_mode_defaults_to_Mipmap)
{
    ASSERT_EQ(Cubemap(1, TextureFormat::RGBA32).filter_mode(), TextureFilterMode::Mipmap);
}

TEST(Cubemap, set_filter_mode_changes_filter_mode)
{
    Cubemap cubemap{1, TextureFormat::RGBA32};
    ASSERT_EQ(cubemap.filter_mode(), TextureFilterMode::Mipmap);
    cubemap.set_filter_mode(TextureFilterMode::Nearest);
    ASSERT_EQ(cubemap.filter_mode(), TextureFilterMode::Nearest);
}

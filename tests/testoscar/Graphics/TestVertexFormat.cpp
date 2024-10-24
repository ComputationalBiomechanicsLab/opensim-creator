#include <oscar/Graphics/VertexFormat.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/VertexAttribute.h>
#include <oscar/Graphics/VertexAttributeDescriptor.h>
#include <oscar/Graphics/VertexAttributeFormat.h>

#include <algorithm>
#include <array>
#include <initializer_list>
#include <ranges>
#include <span>
#include <type_traits>
#include <vector>

using namespace osc;
namespace rgs = std::ranges;

TEST(VertexFormat, is_default_constructible)
{
    static_assert(std::is_default_constructible_v<VertexFormat>);
}

TEST(VertexFormat, can_construct_with_just_a_position_VertexAttribute)
{
    const std::initializer_list<VertexAttributeDescriptor> initializer_list = {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
    };

    ASSERT_NO_THROW({ VertexFormat{initializer_list}; });
}

TEST(VertexFormat, constructor_throws_if_given_two_Position_VertexAttributes)
{
    const std::initializer_list<VertexAttributeDescriptor> initializer_list = {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::Position, VertexAttributeFormat::Unorm8x4},
    };

    ASSERT_ANY_THROW({ VertexFormat{initializer_list}; });
}

TEST(VertexFormat, can_construct_with_many_VertexAttributes_if_they_are_ordered_correctly)
{
    const std::initializer_list<VertexAttributeDescriptor> initializer_list = {
        {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
        {VertexAttribute::Normal,    VertexAttributeFormat::Float32x3},
        {VertexAttribute::Tangent,   VertexAttributeFormat::Unorm8x4},  // nonstandard formats are ok
        {VertexAttribute::Color,     VertexAttributeFormat::Unorm8x4},
        {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
    };

    ASSERT_NO_THROW({ VertexFormat{initializer_list}; });
}

TEST(VertexFormat, constructor_doesnt_throw_if_just_Position_and_Normal)
{
    const std::initializer_list<VertexAttributeDescriptor> initializer_list = {
        {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
        {VertexAttribute::Normal,    VertexAttributeFormat::Float32x3},
    };

    ASSERT_NO_THROW({ VertexFormat{initializer_list}; });
}

TEST(VertexFormat, constructor_throws_if_Position_VertexAttribute_is_missing)
{
    // what this is actually testing is "throws if Postition, in general, is missing"
    //
    // ... it doesn't matter if you provide any/all of the other data

    for (VertexAttribute attr : {VertexAttribute::Normal, VertexAttribute::Tangent, VertexAttribute::Color, VertexAttribute::TexCoord0}) {
        const std::initializer_list<VertexAttributeDescriptor> initializer_list = {
            {attr, VertexAttributeFormat::Float32x3},  // format/dimensionality is flexible w.r.t. the chosen attribute
        };
        ASSERT_ANY_THROW({ VertexFormat{initializer_list}; });
    }
}

TEST(VertexFormat, throws_if_same_VertexAttribute_is_supplied_multiple_times)
{
    // the implementation should throw if the caller provides the same attribute multiple times,
    // because renderer algorithms may assume that the data does not need to be duplicated within
    // one buffer

    const std::initializer_list<VertexAttributeDescriptor> initializer_list = {
        {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
        {VertexAttribute::Normal,    VertexAttributeFormat::Float32x3},
        {VertexAttribute::Tangent,   VertexAttributeFormat::Unorm8x4},
        {VertexAttribute::Color,     VertexAttributeFormat::Unorm8x4},
        {VertexAttribute::Normal,    VertexAttributeFormat::Float32x3},  // uh oh (doesn't matter if it matches)
        {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
    };

    ASSERT_ANY_THROW({ VertexFormat{initializer_list}; });
}

TEST(VertexFormat, clear_makes_it_equivalent_to_default_constructed_VertexFormat)
{
    VertexFormat vertex_format = {
        {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
        {VertexAttribute::Normal,    VertexAttributeFormat::Float32x3},
    };

    vertex_format.clear();

    ASSERT_TRUE(vertex_format.empty());
    ASSERT_EQ(vertex_format, VertexFormat{});
}

TEST(VertexFormat, stride_returns_zero_on_default_construction)
{
    ASSERT_EQ(VertexFormat{}.stride(), 0);
}

TEST(VertexFormat, stride_returns_expected_results)
{
    {
        const VertexFormat vertex_format = {
            {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
            {VertexAttribute::Normal,    VertexAttributeFormat::Float32x3},
        };
        ASSERT_EQ(vertex_format.stride(), 6*sizeof(float));
    }
    {
        const VertexFormat vertex_format = {
            {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
            {VertexAttribute::Normal,    VertexAttributeFormat::Unorm8x4},
        };
        ASSERT_EQ(vertex_format.stride(), 3*sizeof(float)+4);
    }
    {
        const VertexFormat vertex_format = {
            {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
            {VertexAttribute::Normal,    VertexAttributeFormat::Unorm8x4},
            {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
        };
        ASSERT_EQ(vertex_format.stride(), 3*sizeof(float)+4+2*sizeof(float));
    }
}

TEST(VertexFormat, contains_returns_false_on_default_constructed_VertexFormat)
{
    ASSERT_FALSE(VertexFormat{}.contains(VertexAttribute::Position));
    ASSERT_FALSE(VertexFormat{}.contains(VertexAttribute::Color));
    ASSERT_FALSE(VertexFormat{}.contains(VertexAttribute::Tangent));
    // etc.
}

TEST(VertexFormat, contains_returns_false_on_not_contained_Attribute)
{
    const VertexFormat vertex_format = {
        {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
        {VertexAttribute::Normal,    VertexAttributeFormat::Unorm8x4},
        {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
    };

    ASSERT_FALSE(vertex_format.contains(VertexAttribute::Color));
    ASSERT_FALSE(vertex_format.contains(VertexAttribute::Tangent));
}

TEST(VertexFormat, contains_returns_true_on_contained_VertexAttribute)
{
    const VertexFormat vertex_format = {
        {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
        {VertexAttribute::Normal,    VertexAttributeFormat::Unorm8x4},
        {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
    };

    ASSERT_TRUE(vertex_format.contains(VertexAttribute::Position));
    ASSERT_TRUE(vertex_format.contains(VertexAttribute::Normal));
    ASSERT_TRUE(vertex_format.contains(VertexAttribute::TexCoord0));
}

TEST(VertexFormat, attribute_layout_returns_nullopt_for_not_contained_VertexAttribute)
{
    const VertexFormat vertex_format = {
        {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
        {VertexAttribute::Normal,    VertexAttributeFormat::Unorm8x4},
        {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
    };

    ASSERT_EQ(vertex_format.attribute_layout(VertexAttribute::Color), std::nullopt);
    ASSERT_EQ(vertex_format.attribute_layout(VertexAttribute::Tangent), std::nullopt);
}

TEST(VertexFormat, attribute_layout_returns_expected_answers_for_existent_VertexAttribute)
{
    const VertexFormat vertex_format = {
        {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
        {VertexAttribute::Normal,    VertexAttributeFormat::Unorm8x4},
        {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
    };

    using Layout = VertexFormat::VertexAttributeLayout;

    ASSERT_EQ(vertex_format.attribute_layout(VertexAttribute::Position), Layout(VertexAttributeDescriptor(VertexAttribute::Position, VertexAttributeFormat::Float32x3), 0));
    ASSERT_EQ(vertex_format.attribute_layout(VertexAttribute::Normal), Layout(VertexAttributeDescriptor(VertexAttribute::Normal, VertexAttributeFormat::Unorm8x4), 3*sizeof(float)));
    ASSERT_EQ(vertex_format.attribute_layout(VertexAttribute::TexCoord0), Layout(VertexAttributeDescriptor(VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2), 3*sizeof(float)+4));
}

TEST(VertexFormat, attribute_layouts_returns_provided_descriptions_with_expected_offsets)
{
    const VertexFormat vertex_format = {
        {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
        {VertexAttribute::Normal,    VertexAttributeFormat::Unorm8x4},
        {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
    };

    using Layout = VertexFormat::VertexAttributeLayout;

    const auto expected_layouts = std::to_array<Layout>({
        {{VertexAttribute::Position,  VertexAttributeFormat::Float32x3}, 0},
        {{VertexAttribute::Normal,    VertexAttributeFormat::Unorm8x4},  3*sizeof(float)},
        {{VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2}, 3*sizeof(float)+4},
    });

    ASSERT_TRUE(rgs::equal(vertex_format.attribute_layouts(), expected_layouts));
}

TEST(VertexFormat, insert_does_nothing_if_assigning_non_Position_VertexAttribute_if_no_Position_is_available)
{
    VertexFormat vertex_format;
    vertex_format.insert({VertexAttribute::Tangent, VertexAttributeFormat::Float32x4});

    ASSERT_EQ(vertex_format, VertexFormat{});
}

TEST(VertexFormat, insert_works_when_inserting_position_to_an_empty_format)
{
    VertexFormat vertex_format;
    vertex_format.insert({VertexAttribute::Position, VertexAttributeFormat::Float32x3});

    const VertexFormat expected_format = {{VertexAttribute::Position, VertexAttributeFormat::Float32x3}};

    ASSERT_EQ(vertex_format, expected_format);
}

TEST(VertexFormat, insert_works_when_inserting_a_second_VertexAttribute)
{
    VertexFormat vertex_format{{VertexAttribute::Position, VertexAttributeFormat::Float32x3}};
    vertex_format.insert({VertexAttribute::Tangent, VertexAttributeFormat::Unorm8x4});

    const VertexFormat expected_format = {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::Tangent, VertexAttributeFormat::Unorm8x4},
    };

    ASSERT_EQ(vertex_format, expected_format);
}

TEST(VertexFormat, insert_can_insert_a_third_VertexAttribute)
{
    VertexFormat vertex_format = {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::Tangent, VertexAttributeFormat::Unorm8x4},
    };
    vertex_format.insert({VertexAttribute::Normal, VertexAttributeFormat::Float32x3});

    const VertexFormat expected_format = {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::Tangent, VertexAttributeFormat::Unorm8x4},
        {VertexAttribute::Normal, VertexAttributeFormat::Float32x3},
    };

    ASSERT_EQ(vertex_format, expected_format);
}

TEST(VertexFormat, insert_overwrites_existing_VertexAttributes_in_the_VertexFormat)
{
    VertexFormat vertex_format = {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::Tangent, VertexAttributeFormat::Unorm8x4},
    };
    vertex_format.insert({VertexAttribute::Tangent, VertexAttributeFormat::Float32x2});

    const VertexFormat expected_format = {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::Tangent, VertexAttributeFormat::Float32x2},
    };

    ASSERT_EQ(vertex_format, expected_format);
}

TEST(VertexFormat, erase_non_contained_VertexAttribute_does_nothing)
{
    const VertexFormat vertex_format_before = {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::Tangent, VertexAttributeFormat::Unorm8x4},
    };
    VertexFormat vertex_format_after = vertex_format_before;
    vertex_format_after.erase(VertexAttribute::Color);

    ASSERT_EQ(vertex_format_after, vertex_format_before);
}

TEST(VertexFormat, erase_esrases_contained_VertexAttributes)
{
    VertexFormat vertex_format = {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::Tangent, VertexAttributeFormat::Unorm8x4},
    };
    vertex_format.erase(VertexAttribute::Tangent);
    const VertexFormat expected_format = {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
    };
    ASSERT_EQ(vertex_format, expected_format);
}

TEST(VertexFormat, erase_Position_wipes_all_VertexAttributes)
{
    // because the Position attribute is required by all formats, you
    // shouldn't be able to delete it and leave the remainder "dangling"

    VertexFormat vertex_format = {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::Tangent, VertexAttributeFormat::Unorm8x4},
    };
    vertex_format.erase(VertexAttribute::Position);
    ASSERT_EQ(vertex_format, VertexFormat{});
}

TEST(VertexFormat, retains_caller_provided_layout)
{
    // because the caller might be setting up a buffer with a very specific
    // layout, the `VertexFormat` shouldn't shuffle the non-`Position` fields
    // around at all
    std::vector<VertexAttributeDescriptor> attribute_descriptions = {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},  // required
        {VertexAttribute::Normal, VertexAttributeFormat::Float32x2},
        {VertexAttribute::Tangent, VertexAttributeFormat::Unorm8x4},
        {VertexAttribute::Color, VertexAttributeFormat::Float32x4},
        {VertexAttribute::TexCoord0, VertexAttributeFormat::Unorm8x4},
    };

    // permute the non-Position fields
    const auto fields_to_permute = std::span{attribute_descriptions}.subspan(1);
    for (bool permuted = true; permuted; permuted = rgs::next_permutation(fields_to_permute, rgs::less{}, &VertexAttributeDescriptor::attribute).found) {

        const VertexFormat permutation_format{attribute_descriptions};

        ASSERT_TRUE(rgs::equal(
            attribute_descriptions,
            permutation_format.attribute_layouts(),
            rgs::equal_to{},
            &VertexAttributeDescriptor::attribute,
            &VertexFormat::VertexAttributeLayout::attribute
        ));
    }
}

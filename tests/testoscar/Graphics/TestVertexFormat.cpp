#include <oscar/Graphics/VertexFormat.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/VertexAttribute.h>
#include <oscar/Graphics/VertexAttributeDescriptor.h>
#include <oscar/Graphics/VertexAttributeFormat.h>

#include <array>
#include <initializer_list>

using namespace osc;

TEST(VertexFormat, IsDefaultConstructible)
{
    ASSERT_NO_THROW({ VertexFormat{}; });
}

TEST(VertexFormat, CanConstructWithJustAPositionAttribute)
{
    std::initializer_list<VertexAttributeDescriptor> const lst = {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
    };

    ASSERT_NO_THROW({ VertexFormat{lst}; });
}

TEST(VertexFormat, ThrowsIfGivenDuplicatePositionAttributes)
{
    std::initializer_list<VertexAttributeDescriptor> const lst = {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::Position, VertexAttributeFormat::Unorm8x4},
    };

    ASSERT_ANY_THROW({ VertexFormat{lst}; });
}

TEST(VertexFormat, CanHandleManyAttributesIfOrderedCorrectly)
{
    std::initializer_list<VertexAttributeDescriptor> const lst = {
        {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
        {VertexAttribute::Normal,    VertexAttributeFormat::Float32x3},
        {VertexAttribute::Tangent,   VertexAttributeFormat::Unorm8x4},  // nonstandard formats are ok
        {VertexAttribute::Color,     VertexAttributeFormat::Unorm8x4},
        {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
    };

    ASSERT_NO_THROW({ VertexFormat{lst}; });
}

TEST(VertexFormat, DoesNotThrowIfJustPositionAndNormal)
{
    std::initializer_list<VertexAttributeDescriptor> const lst = {
        {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
        {VertexAttribute::Normal,    VertexAttributeFormat::Float32x3},
    };

    ASSERT_NO_THROW({ VertexFormat{lst}; });
}

TEST(VertexFormat, ThrowsIfPositionIsMissing)
{
    // what this is actually testing is "throws if Postition, in general, is missing"
    //
    // ... it doesn't matter if you provide any/all of the other data

    for (VertexAttribute a : {VertexAttribute::Normal, VertexAttribute::Tangent, VertexAttribute::Color, VertexAttribute::TexCoord0})
    {
        std::initializer_list<VertexAttributeDescriptor> const lst = {
            {a, VertexAttributeFormat::Float32x3},  // format/dimensionality is flexible w.r.t. the chosen attribute
        };
        ASSERT_ANY_THROW({ VertexFormat{lst}; });
    }
}

TEST(VertexFormat, ThrowsIfSameAttributeSuppliedMultipleTimes)
{
    // the implementation should throw if the caller provides the same attribute multiple times,
    // because renderer algorithms may assume that the data does not need to be duplicated within
    // one buffer

    std::initializer_list<VertexAttributeDescriptor> const lst = {
        {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
        {VertexAttribute::Normal,    VertexAttributeFormat::Float32x3},
        {VertexAttribute::Tangent,   VertexAttributeFormat::Unorm8x4},
        {VertexAttribute::Color,     VertexAttributeFormat::Unorm8x4},
        {VertexAttribute::Normal,    VertexAttributeFormat::Float32x3},  // uh oh (doesn't matter if it matches)
        {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
    };

    ASSERT_ANY_THROW({ VertexFormat{lst}; });
}

TEST(VertexFormat, ClearMakesFormatEquivalentToEmptyFormat)
{
    VertexFormat f = {
        {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
        {VertexAttribute::Normal,    VertexAttributeFormat::Float32x3},
    };

    f.clear();

    ASSERT_TRUE(f.empty());
    ASSERT_EQ(f, VertexFormat{});
}

TEST(VertexFormat, StrideReturnsZeroOnDefaultConstruction)
{
    ASSERT_EQ(VertexFormat{}.stride(), 0);
}

TEST(VertexFormat, StrideReturnsExpectedResults)
{
    {
        VertexFormat const f
        {
            {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
            {VertexAttribute::Normal,    VertexAttributeFormat::Float32x3},
        };
        ASSERT_EQ(f.stride(), 6*sizeof(float));
    }
    {
        VertexFormat const f
        {
            {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
            {VertexAttribute::Normal,    VertexAttributeFormat::Unorm8x4},
        };
        ASSERT_EQ(f.stride(), 3*sizeof(float)+4);
    }
    {
        VertexFormat const f
        {
            {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
            {VertexAttribute::Normal,    VertexAttributeFormat::Unorm8x4},
            {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
        };
        ASSERT_EQ(f.stride(), 3*sizeof(float)+4+2*sizeof(float));
    }
}

TEST(VertexFormat, ContainsReturnsFalseOnEmptyFormat)
{
    ASSERT_FALSE(VertexFormat{}.contains(VertexAttribute::Position));
    ASSERT_FALSE(VertexFormat{}.contains(VertexAttribute::Color));
    ASSERT_FALSE(VertexFormat{}.contains(VertexAttribute::Tangent));
    // etc.
}

TEST(VertexFormat, ContainsReturnsFalseOnNonExistentAttr)
{
    VertexFormat const f
    {
        {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
        {VertexAttribute::Normal,    VertexAttributeFormat::Unorm8x4},
        {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
    };

    ASSERT_FALSE(f.contains(VertexAttribute::Color));
    ASSERT_FALSE(f.contains(VertexAttribute::Tangent));
}

TEST(VertexFormat, ContainsReturnsTrueOnExistentAttr)
{
    VertexFormat const f
    {
        {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
        {VertexAttribute::Normal,    VertexAttributeFormat::Unorm8x4},
        {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
    };

    ASSERT_TRUE(f.contains(VertexAttribute::Position));
    ASSERT_TRUE(f.contains(VertexAttribute::Normal));
    ASSERT_TRUE(f.contains(VertexAttribute::TexCoord0));
}

TEST(VertexFormat, AttributeLayoutReturnsNulloptForNonExistentAttribute)
{
    VertexFormat const f
    {
        {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
        {VertexAttribute::Normal,    VertexAttributeFormat::Unorm8x4},
        {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
    };

    ASSERT_EQ(f.attributeLayout(VertexAttribute::Color), std::nullopt);
    ASSERT_EQ(f.attributeLayout(VertexAttribute::Tangent), std::nullopt);
}

TEST(VertexFormat, AttributeLayoutReturnsExpectedAnswersForExistentAttribute)
{
    VertexFormat const f
    {
        {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
        {VertexAttribute::Normal,    VertexAttributeFormat::Unorm8x4},
        {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
    };

    using Layout = VertexFormat::VertexAttributeLayout;

    ASSERT_EQ(f.attributeLayout(VertexAttribute::Position), Layout(VertexAttributeDescriptor(VertexAttribute::Position, VertexAttributeFormat::Float32x3), 0));
    ASSERT_EQ(f.attributeLayout(VertexAttribute::Normal), Layout(VertexAttributeDescriptor(VertexAttribute::Normal, VertexAttributeFormat::Unorm8x4), 3*sizeof(float)));
    ASSERT_EQ(f.attributeLayout(VertexAttribute::TexCoord0), Layout(VertexAttributeDescriptor(VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2), 3*sizeof(float)+4));
}

TEST(VertexFormat, AttributeLayoutsReturnsProvidedAttributeDescriptionsWithOffsets)
{
    VertexFormat const f
    {
        {VertexAttribute::Position,  VertexAttributeFormat::Float32x3},
        {VertexAttribute::Normal,    VertexAttributeFormat::Unorm8x4},
        {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
    };

    using Layout = VertexFormat::VertexAttributeLayout;

    auto const expected = std::to_array<Layout>(
    {
        {{VertexAttribute::Position,  VertexAttributeFormat::Float32x3}, 0},
        {{VertexAttribute::Normal,    VertexAttributeFormat::Unorm8x4},  3*sizeof(float)},
        {{VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2}, 3*sizeof(float)+4},
    });

    auto equal = [](auto const& range1, auto const& range2)
    {
        return std::equal(range1.begin(), range1.end(), range2.begin(), range2.end());
    };


    ASSERT_TRUE(equal(f.attributeLayouts(), expected));
}

TEST(VertexFormat, InsertDoesNothingIfAssigningNonPositionAttributeIfNoPositionAttributeIsAvailable)
{
    VertexFormat f;
    f.insert({VertexAttribute::Tangent, VertexAttributeFormat::Float32x4});

    ASSERT_EQ(f, VertexFormat{});
}

TEST(VertexFormat, InsertWorksWhenInsertingPositionToAnEmptyFormat)
{
    VertexFormat f;
    f.insert({VertexAttribute::Position, VertexAttributeFormat::Float32x3});

    VertexFormat const expected = {{VertexAttribute::Position, VertexAttributeFormat::Float32x3}};

    ASSERT_EQ(f, expected);
}

TEST(VertexFormat, InsertWorksWhenInsertingASecondAttribute)
{
    VertexFormat f{{VertexAttribute::Position, VertexAttributeFormat::Float32x3}};
    f.insert({VertexAttribute::Tangent, VertexAttributeFormat::Unorm8x4});

    VertexFormat const expected =
    {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::Tangent, VertexAttributeFormat::Unorm8x4},
    };

    ASSERT_EQ(f, expected);
}

TEST(VertexFormat, InsertCanInsertElementsInBetweenExistingElements)
{
    VertexFormat f =
    {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::Tangent, VertexAttributeFormat::Unorm8x4},
    };
    f.insert({VertexAttribute::Normal, VertexAttributeFormat::Float32x3});

    VertexFormat const expected =
    {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::Normal, VertexAttributeFormat::Float32x3},
        {VertexAttribute::Tangent, VertexAttributeFormat::Unorm8x4},
    };

    ASSERT_EQ(f, expected);
}

TEST(VertexFormat, InsertOverwritesExistingAttributesFormatIfAlreadyInFormat)
{
    VertexFormat f =
    {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::Tangent, VertexAttributeFormat::Unorm8x4},
    };
    f.insert({VertexAttribute::Tangent, VertexAttributeFormat::Float32x2});

    VertexFormat const expected =
    {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::Tangent, VertexAttributeFormat::Float32x2},
    };

    ASSERT_EQ(f, expected);
}

TEST(VertexFormat, EraseWithNonPresentVertexAttributeDoesNothing)
{
    VertexFormat const before =
    {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::Tangent, VertexAttributeFormat::Unorm8x4},
    };
    VertexFormat after = before;
    after.erase(VertexAttribute::Color);

    ASSERT_EQ(after, before);
}

TEST(VertexFormat, EraseErasesExistentAttribute)
{
    VertexFormat f =
    {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::Tangent, VertexAttributeFormat::Unorm8x4},
    };
    f.erase(VertexAttribute::Tangent);
    VertexFormat const expected =
    {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
    };
    ASSERT_EQ(f, expected);
}

TEST(VertexFormat, ErasePositionWipesAllAttributes)
{
    VertexFormat f =
    {
        {VertexAttribute::Position, VertexAttributeFormat::Float32x3},
        {VertexAttribute::Tangent, VertexAttributeFormat::Unorm8x4},
    };
    f.erase(VertexAttribute::Position);
    ASSERT_EQ(f, VertexFormat{});
}

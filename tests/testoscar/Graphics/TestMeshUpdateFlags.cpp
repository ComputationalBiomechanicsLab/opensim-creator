#include <oscar/Graphics/MeshUpdateFlags.h>

#include <gtest/gtest.h>

using osc::MeshUpdateFlags;

TEST(MeshUpdateFlags, ORingCombinesFlags)
{
    static_assert((MeshUpdateFlags::DontRecalculateBounds | MeshUpdateFlags::DontValidateIndices) & MeshUpdateFlags::DontRecalculateBounds);
    static_assert((MeshUpdateFlags::DontRecalculateBounds | MeshUpdateFlags::DontValidateIndices) & MeshUpdateFlags::DontValidateIndices);
}

TEST(MeshUpdateFlags, ANDingChecksFlags)
{
    static_assert(!(MeshUpdateFlags::Default & MeshUpdateFlags::Default));  // NOLINT(misc-redundant-expression)
    static_assert(!(MeshUpdateFlags::Default & MeshUpdateFlags::DontRecalculateBounds));
    static_assert(!(MeshUpdateFlags::Default & MeshUpdateFlags::DontValidateIndices));

    static_assert(!(MeshUpdateFlags::DontRecalculateBounds & MeshUpdateFlags::Default));
    static_assert(MeshUpdateFlags::DontRecalculateBounds & MeshUpdateFlags::DontRecalculateBounds);  // NOLINT(misc-redundant-expression)
    static_assert(!(MeshUpdateFlags::DontRecalculateBounds & MeshUpdateFlags::DontValidateIndices));

    static_assert(!(MeshUpdateFlags::DontValidateIndices & MeshUpdateFlags::Default));
    static_assert(!(MeshUpdateFlags::DontValidateIndices & MeshUpdateFlags::DontRecalculateBounds));
    static_assert(MeshUpdateFlags::DontValidateIndices & MeshUpdateFlags::DontValidateIndices);  // NOLINT(misc-redundant-expression)
}

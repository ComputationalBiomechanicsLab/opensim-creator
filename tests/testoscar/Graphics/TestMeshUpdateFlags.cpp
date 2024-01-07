#include <oscar/Graphics/MeshUpdateFlags.hpp>

#include <gtest/gtest.h>

using osc::MeshUpdateFlags;

TEST(MeshUpdateFlags, ORingCombinesFlags)
{
    static_assert((MeshUpdateFlags::DontRecalculateBounds | MeshUpdateFlags::DontValidateIndices) & MeshUpdateFlags::DontRecalculateBounds);
    static_assert((MeshUpdateFlags::DontRecalculateBounds | MeshUpdateFlags::DontValidateIndices) & MeshUpdateFlags::DontValidateIndices);
}

TEST(MeshUpdateFlags, ANDingChecksFlags)
{
    static_assert((MeshUpdateFlags::Default & MeshUpdateFlags::Default) == false);
    static_assert((MeshUpdateFlags::Default & MeshUpdateFlags::DontRecalculateBounds) == false);
    static_assert((MeshUpdateFlags::Default & MeshUpdateFlags::DontValidateIndices) == false);

    static_assert((MeshUpdateFlags::DontRecalculateBounds & MeshUpdateFlags::Default) == false);
    static_assert((MeshUpdateFlags::DontRecalculateBounds & MeshUpdateFlags::DontRecalculateBounds) == true);
    static_assert((MeshUpdateFlags::DontRecalculateBounds & MeshUpdateFlags::DontValidateIndices) == false);

    static_assert((MeshUpdateFlags::DontValidateIndices & MeshUpdateFlags::Default) == false);
    static_assert((MeshUpdateFlags::DontValidateIndices & MeshUpdateFlags::DontRecalculateBounds) == false);
    static_assert((MeshUpdateFlags::DontValidateIndices & MeshUpdateFlags::DontValidateIndices) == true);
}

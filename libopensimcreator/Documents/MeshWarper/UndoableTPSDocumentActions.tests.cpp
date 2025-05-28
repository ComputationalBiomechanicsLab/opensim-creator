#include "UndoableTPSDocumentActions.h"

#include <gtest/gtest.h>
#include <libopensimcreator/Documents/MeshWarper/TPSDocumentLandmarkPair.h>

#include <algorithm>
#include <array>
#include <sstream>

using namespace osc;
namespace rgs = std::ranges;

TEST(ActionWriteLandmarksAsCSV, worksOnBasicExample)
{
    const auto pairs = std::to_array<TPSDocumentLandmarkPair>({
        TPSDocumentLandmarkPair{"p1", std::nullopt,           std::nullopt},
        TPSDocumentLandmarkPair{"p2", Vec3{},                 Vec3{1.0f, 1.0f, 1.0f}},
        TPSDocumentLandmarkPair{"p3", Vec3{1.0f, 0.0f, 0.0f}, std::nullopt},
        TPSDocumentLandmarkPair{"p4", Vec3{2.0f, 1.0f, 0.0f}, Vec3{0.0f, -1.0f, -2.0f}},
        TPSDocumentLandmarkPair{"p5", Vec3{2.0f, 1.0f, 0.0f}, Vec3{}},
    });

    std::stringstream ss;
    ActionWriteLandmarksAsCSV(pairs, TPSDocumentInputIdentifier::Source, lm::LandmarkCSVFlags::None, ss);
    const std::string output = std::move(ss).str();

    ASSERT_FALSE(output.empty());
    for (auto const& header : {"p2", "p3", "p4", "p5"}) {
        ASSERT_TRUE(output.contains(header));
    }

    ASSERT_EQ(rgs::count(output, '\n'), 5);
}

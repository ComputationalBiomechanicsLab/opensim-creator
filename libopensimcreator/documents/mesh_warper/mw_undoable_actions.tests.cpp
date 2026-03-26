#include "mw_undoable_actions.h"

#include <gtest/gtest.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_landmark_pair.h>

#include <algorithm>
#include <array>
#include <sstream>

using namespace osc;
namespace rgs = std::ranges;

TEST(ActionWriteLandmarksAsCSV, worksOnBasicExample)
{
    const auto pairs = std::to_array<MwDocumentLandmarkPair>({
        MwDocumentLandmarkPair{"p1", std::nullopt,              std::nullopt},
        MwDocumentLandmarkPair{"p2", Vector3{},                 Vector3{1.0f, 1.0f, 1.0f}},
        MwDocumentLandmarkPair{"p3", Vector3{1.0f, 0.0f, 0.0f}, std::nullopt},
        MwDocumentLandmarkPair{"p4", Vector3{2.0f, 1.0f, 0.0f}, Vector3{0.0f, -1.0f, -2.0f}},
        MwDocumentLandmarkPair{"p5", Vector3{2.0f, 1.0f, 0.0f}, Vector3{}},
    });

    std::stringstream ss;
    ActionWriteLandmarksAsCSV(pairs, MiDocumentInputIdentifier::Source, opyn::LandmarkCSVFlags::None, ss);
    const std::string output = std::move(ss).str();

    ASSERT_FALSE(output.empty());
    for (auto const& header : {"p2", "p3", "p4", "p5"}) {
        ASSERT_TRUE(output.contains(header));
    }

    ASSERT_EQ(rgs::count(output, '\n'), 5);
}

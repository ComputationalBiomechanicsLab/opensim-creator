#include "LandmarkHelpers.h"

#include <libopensimcreator/testing/TestOpenSimCreatorConfig.h>

#include <gtest/gtest.h>
#include <liboscar/Maths/Vector3.h>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace osc;
using namespace osc::lm;

namespace
{
    std::filesystem::path GetFixturesDir()
    {
        auto p = std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "Document/Landmarks";
        p = std::filesystem::weakly_canonical(p);
        return p;
    }

    std::ifstream OpenFixtureFile(std::string_view fixtureName)
    {
        const std::filesystem::path path = GetFixturesDir() / fixtureName;

        std::ifstream f{path};
        if (!f)
        {
            std::stringstream ss;
            ss << path << ": cannot open fixture path";
            throw std::runtime_error{std::move(ss).str()};
        }
        return f;
    }

    template<class T>
    auto VectorReadingIterator(const std::vector<T>& vs)
    {
        return [it = vs.begin(), end = vs.end()]() mutable
        {
            if (it != end)
            {

                return std::optional<T>{*it++};
            }
            else
            {
                return std::optional<T>{};
            }
        };
    }
}

// edge-case
TEST(LandmarkHelpers, ReadLandmarksFromCSVReturnsNoRowsForBlankCSV)
{
    auto input = OpenFixtureFile("blank.csv");
    size_t i = 0;
    ReadLandmarksFromCSV(input, [&i](auto&&){ ++i; });
    ASSERT_EQ(i, 0);
}

// this is what early versions of the mesh warper used to export. Later versions
// expose a similar format through (e.g.) "Export Landmark _positions_" for backwards
// compat
TEST(LandmarkHelpers, ReadLandmarksFromCSVWorksFor3ColCSVWithNoHeader)
{
    auto input = OpenFixtureFile("3colnoheader.csv");
    size_t i = 0;
    ReadLandmarksFromCSV(input, [&i](auto&&){ ++i; });
    ASSERT_EQ(i, 4);
}

// iirc, this isn't exported by OSC directly but is an entirely reasonable thing to
// expect users to supply to the software
TEST(LandmarkHelpers, ReadLandmarksFromCSVWorksFor3ColCSVWithHeader)
{
    auto input = OpenFixtureFile("3colwithheader.csv");
    size_t i = 0;
    ReadLandmarksFromCSV(input, [&i](auto&&){ ++i; });
    ASSERT_EQ(i, 4);  // (skipped the header)
}

// invalid rows that don't contain three columns of numeric data are ultimately ignored
TEST(LandmarkHelpers, ReadLandmarksFromCSVIgnoresInvalidRows)
{
    auto input = OpenFixtureFile("3colbutinvalid.csv");
    size_t i = 0;
    ReadLandmarksFromCSV(input, [&i](auto&&){ ++i; });
    ASSERT_EQ(i, 0);  // (skipped all rows)
}

// although this is technically a bodged file, it's one of those things that custom python
// scripts might spit out, or users might want blank lines in their CSV as a primitive way
// of grouping datapoints - just ignore the whole row
TEST(LandmarkHelpers, ReadLandmarksFromCSVContainingSparseErrorsAndBlankRowsJustIgnoresThem)
{
    auto input = OpenFixtureFile("3colsparseerrors.csv");
    size_t i = 0;
    ReadLandmarksFromCSV(input, [&i](auto&&){ ++i; });
    ASSERT_EQ(i, 4);  // (skipped the bad ones)
}

// this is what the mesh warper etc. tend to export: 4 columns, with the first being a name column
TEST(LandmarkHelpers, ReadLandmarksFromTypical4ColumnCSVWorksAsExpected)
{
    auto input = OpenFixtureFile("4column.csv");
    std::vector<std::string> names;
    ReadLandmarksFromCSV(input, [&names](auto&& lm)
    {
        if (lm.maybeName)
        {
            names.push_back(std::move(lm.maybeName).value());
        }
    });
    std::vector<std::string> expectedNames =
    {
        "landmark_0",
        "landmark_1",
        "landmark_2",
        "landmark_3",
        "landmark_4",
        "landmark_5",
        "landmark_6",
    };

    ASSERT_EQ(names, expectedNames);
}

// if a CSV file contains additional columns, ignore them for now
TEST(LandmarkHelpers, ReadLandmarksFromOver4ColumnCSVIgnoresTrailingColumns)
{
    auto input = OpenFixtureFile("6column.csv");
    size_t i = 0;
    ReadLandmarksFromCSV(input, [&i](auto&&) { ++i; });
    ASSERT_EQ(i, 7);
}

TEST(LandmarkHelpers, WriteLandmarksToCSVWritesHeaderRowWhenGivenBlankData)
{
    const std::vector<Landmark> landmarks = {};
    std::stringstream out;
    WriteLandmarksToCSV(out, VectorReadingIterator(landmarks));

    ASSERT_EQ(out.str(), "name,x,y,z\n");
}

TEST(LandmarkHelpers, WriteLandmarksToCSVWritesNothingWhenNoHeaderRowIsRequested)
{
    const std::vector<Landmark> landmarks = {};
    std::stringstream out;
    WriteLandmarksToCSV(out, VectorReadingIterator(landmarks), LandmarkCSVFlags::NoHeader);

    ASSERT_EQ(out.str(), "");
}

TEST(LandmarkHelpers, WriteLandmarksToCSVWritesOnlyXYZIfNoNameRequested)
{
    const std::vector<Landmark> landmarks = {};
    std::stringstream out;
    WriteLandmarksToCSV(out, VectorReadingIterator(landmarks), LandmarkCSVFlags::NoNames);

    ASSERT_EQ(out.str(), "x,y,z\n");
}

TEST(LandmarkHelpers, GenerateNamesDoesNotChangeInputIfInputIsFullyNamed)
{
    const std::vector<Landmark> input = {
        {"p1",   {}},
        {"p2",   {0.0f, 1.0f, 0.0f}},
        {"etc.", {1.0f, 1.0f, 0.0f}},
    };
    const auto output = GenerateNames(input);

    ASSERT_TRUE(std::equal(output.begin(), output.end(), input.begin(), input.end()));
}

TEST(LandmarkHelpers, GenerateNamesGeneratesPrefixedNameForUnnamedInputs)
{
    const std::vector<Landmark> input = {
        {"p1",         {}},
        {std::nullopt, {0.0f, 1.0f, 0.0f}},
        {"etc.",       {1.0f, 1.0f, 0.0f}},
    };
    const std::vector<NamedLandmark> expectedOutput = {
        {"p1", Vector3{}},
        {"someprefix_0", Vector3{0.0f, 1.0f, 0.0f}},
        {"etc.", Vector3{1.0f, 1.0f, 0.0f}},
    };
    const auto output = GenerateNames(input, "someprefix_");

    ASSERT_TRUE(std::equal(output.begin(), output.end(), expectedOutput.begin(), expectedOutput.end()));
}

TEST(LandmarkHelpers, GenerateNamesBehavesAsExpectedInPathologicalCase)
{
    const std::vector<Landmark> input = {
        {"p1",           {}},
        {std::nullopt,   {0.0f, 1.0f, 0.0f}},
        {"someprefix_0", {1.0f, 1.0f, 0.0f}},  // uh oh
        {"someprefix_1", {2.0f, 0.0f, 0.0f}},  // uhhhh oh
        {std::nullopt,   {}},
    };
    const std::vector<NamedLandmark> expectedOutput = {
        {"p1",           {}},
        {"someprefix_2", {0.0f, 1.0f, 0.0f}},
        {"someprefix_0", {1.0f, 1.0f, 0.0f}},
        {"someprefix_1", {2.0f, 0.0f, 0.0f}},
        {"someprefix_3", {}},
    };
    const auto output = GenerateNames(input, "someprefix_");

    ASSERT_TRUE(std::equal(output.begin(), output.end(), expectedOutput.begin(), expectedOutput.end()));
}

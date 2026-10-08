#include "landmark_helpers.h"

#include <libopynsim/tests/opynsim_tests_config.h>

#include <gtest/gtest.h>
#include <liboscar/maths/vector.h>
#include <liboscar/utilities/exception_helpers.h>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace opyn;
namespace rgs = std::ranges;

namespace
{
    std::filesystem::path get_fixtures_dir()
    {
        auto p = std::filesystem::path{OPYNSIM_TESTS_RESOURCES_DIR} / "Documents/Landmarks";
        p = std::filesystem::weakly_canonical(p);
        return p;
    }

    std::ifstream open_fixture_file(std::string_view fixture_name)
    {
        const std::filesystem::path path = get_fixtures_dir() / fixture_name;

        std::ifstream f{path};
        if (not f) {
            throw osc::formatted_runtime_error("{}: cannot open fixture path", path.string());
        }
        return f;
    }

    template<class T>
    auto vector_reading_iterator(const std::vector<T>& vs)
    {
        return [it = vs.begin(), end = vs.end()]() mutable
        {
            if (it != end) {

                return std::optional<T>{*it++};
            } else {
                return std::optional<T>{};
            }
        };
    }
}

// edge-case
TEST(LandmarkHelpers, read_landmarks_from_csv_returns_no_rows_for_blank_csv)
{
    auto input = open_fixture_file("blank.csv");
    size_t i = 0;
    read_landmarks_from_csv(input, [&i](auto&&){ ++i; });
    ASSERT_EQ(i, 0);
}

// this is what early versions of the mesh warper used to export. Later versions
// expose a similar format through (e.g.) "Export Landmark _positions_" for backwards
// compat
TEST(LandmarkHelpers, read_landmarks_from_csv_works_for_3_col_csv_with_no_header)
{
    auto input = open_fixture_file("3colnoheader.csv");
    size_t i = 0;
    read_landmarks_from_csv(input, [&i](auto&&){ ++i; });
    ASSERT_EQ(i, 4);
}

// iirc, this isn't exported by OSC directly but is an entirely reasonable thing to
// expect users to supply to the software
TEST(LandmarkHelpers, read_landmarks_from_csv_works_for_3_col_csv_with_header)
{
    auto input = open_fixture_file("3colwithheader.csv");
    size_t i = 0;
    read_landmarks_from_csv(input, [&i](auto&&){ ++i; });
    ASSERT_EQ(i, 4);  // (skipped the header)
}

// invalid rows that don't contain three columns of numeric data are ultimately ignored
TEST(LandmarkHelpers, read_landmarks_from_csv_ignores_invalid_rows)
{
    auto input = open_fixture_file("3colbutinvalid.csv");
    size_t i = 0;
    read_landmarks_from_csv(input, [&i](auto&&){ ++i; });
    ASSERT_EQ(i, 0);  // (skipped all rows)
}

// although this is technically a bodged file, it's one of those things that custom python
// scripts might spit out, or users might want blank lines in their CSV as a primitive way
// of grouping datapoints - just ignore the whole row
TEST(LandmarkHelpers, read_landmarks_from_csv_containing_sparse_errors_and_blank_rows_just_ignores_them)
{
    auto input = open_fixture_file("3colsparseerrors.csv");
    size_t i = 0;
    read_landmarks_from_csv(input, [&i](auto&&){ ++i; });
    ASSERT_EQ(i, 4);  // (skipped the bad ones)
}

// this is what the mesh warper etc. tend to export: 4 columns, with the first being a name column
TEST(LandmarkHelpers, read_landmarks_from_typical_4_column_csv_works_as_expected)
{
    auto input = open_fixture_file("4column.csv");
    std::vector<std::string> names;
    read_landmarks_from_csv(input, [&names](auto&& lm)
    {
        if (lm.maybe_name) {
            names.push_back(std::move(lm.maybe_name).value());
        }
    });
    std::vector<std::string> expected_names =
    {
        "landmark_0",
        "landmark_1",
        "landmark_2",
        "landmark_3",
        "landmark_4",
        "landmark_5",
        "landmark_6",
    };

    ASSERT_EQ(names, expected_names);
}

// if a CSV file contains additional columns, ignore them for now
TEST(LandmarkHelpers, read_landmarks_from_over_4_column_csv_ignores_trailing_columns)
{
    auto input = open_fixture_file("6column.csv");
    size_t i = 0;
    read_landmarks_from_csv(input, [&i](auto&&) { ++i; });
    ASSERT_EQ(i, 7);
}

TEST(LandmarkHelpers, write_landmarks_to_csv_writes_header_row_when_given_blank_data)
{
    const std::vector<Landmark> landmarks = {};
    std::stringstream out;
    write_landmarks_to_csv(out, vector_reading_iterator(landmarks));

    ASSERT_EQ(out.str(), "name,x,y,z\n");
}

TEST(LandmarkHelpers, write_landmarks_to_csv_writes_nothing_when_no_header_row_is_requested)
{
    const std::vector<Landmark> landmarks = {};
    std::stringstream out;
    write_landmarks_to_csv(out, vector_reading_iterator(landmarks), LandmarkCSVFlags::NoHeader);

    ASSERT_EQ(out.str(), "");
}

TEST(LandmarkHelpers, write_landmarks_to_csv_writes_only_xyz_if_no_name_requested)
{
    const std::vector<Landmark> landmarks = {};
    std::stringstream out;
    write_landmarks_to_csv(out, vector_reading_iterator(landmarks), LandmarkCSVFlags::NoNames);

    ASSERT_EQ(out.str(), "x,y,z\n");
}

TEST(LandmarkHelpers, generate_names_does_not_change_input_if_input_is_fully_named)
{
    const std::vector<Landmark> input = {
        {"p1",   {}},
        {"p2",   {0.0f, 1.0f, 0.0f}},
        {"etc.", {1.0f, 1.0f, 0.0f}},
    };
    const auto output = generate_names(input);

    ASSERT_TRUE(rgs::equal(output, input, std::equal_to{}));
}

TEST(LandmarkHelpers, generate_names_generates_prefixed_name_for_unnamed_inputs)
{
    const std::vector<Landmark> input = {
        {"p1",         {}},
        {std::nullopt, {0.0f, 1.0f, 0.0f}},
        {"etc.",       {1.0f, 1.0f, 0.0f}},
    };
    const std::vector<NamedLandmark> expected_output = {
        {"p1",           osc::Vector3{}},
        {"someprefix_0", osc::Vector3{0.0f, 1.0f, 0.0f}},
        {"etc.",         osc::Vector3{1.0f, 1.0f, 0.0f}},
    };
    const auto output = generate_names(input, "someprefix_");

    ASSERT_TRUE(rgs::equal(output, expected_output, std::equal_to{}));
}

TEST(LandmarkHelpers, generate_names_behaves_as_expected_in_pathological_case)
{
    const std::vector<Landmark> input = {
        {"p1",           {}},
        {std::nullopt,   {0.0f, 1.0f, 0.0f}},
        {"someprefix_0", {1.0f, 1.0f, 0.0f}},  // uh oh
        {"someprefix_1", {2.0f, 0.0f, 0.0f}},  // uhhhh oh
        {std::nullopt,   {}},
    };
    const std::vector<NamedLandmark> expected_output = {
        {"p1",           {}},
        {"someprefix_2", {0.0f, 1.0f, 0.0f}},
        {"someprefix_0", {1.0f, 1.0f, 0.0f}},
        {"someprefix_1", {2.0f, 0.0f, 0.0f}},
        {"someprefix_3", {}},
    };
    const auto output = generate_names(input, "someprefix_");

    ASSERT_TRUE(rgs::equal(output, expected_output, std::equal_to{}));
}

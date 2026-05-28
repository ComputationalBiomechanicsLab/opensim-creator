#include "opynsim.h"

#include <libopynsim/tests/opynsim_tests_config.h>
#include <libopynsim/model_specification.h>

#include <gtest/gtest.h>
#include <liboscar/graphics/color.h>
#include <liboscar/graphics/mesh.h>
#include <liboscar/graphics/mesh_topology.h>
#include <liboscar/graphics/texture2d.h>
#include <liboscar/graphics/texture_format.h>
#include <liboscar/maths/vector.h>
#include <liboscar/utilities/string_helpers.h>

#include <array>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

using namespace opyn;

TEST(opynsim, read_osim_throws_if_file_doesnt_exist)
{
    opyn::init();

    ASSERT_ANY_THROW({ read_osim("/this/probably/doesnt/exist"); });
}

TEST(opynsim, read_osim_works_when_given_a_file_that_does_exist)
{
    opyn::init();

    ASSERT_NO_THROW({ read_osim(opynsim_tests_resources_directory() / "models/Blank/blank.osim"); });
}

TEST(opynsim, read_sto_throws_when_given_a_file_that_doesnt_exist)
{
    opyn::init();

    ASSERT_ANY_THROW({ read_sto("/this/probably/doesnt/exist"); });
}

TEST(opynsim, read_sto_doesnt_throw_when_given_a_valid_sto_file)
{
    opyn::init();

    ASSERT_NO_THROW({ read_sto(opynsim_tests_resources_directory() / "Documents/sto/minimal.sto");  });
}

TEST(opynsim, read_sto_columns_returns_time_column_for_minimal_example)
{
    opyn::init();

    const DataFrame df = read_sto(opynsim_tests_resources_directory() / "Documents/sto/minimal.sto");
    const std::vector<std::string> expected = {"time"};

    ASSERT_EQ(df.columns(), expected);
}

TEST(opynsim, read_sto_shape_returns_0_1_for_minimal_example)
{
    opyn::init();

    const DataFrame df = read_sto(opynsim_tests_resources_directory() / "Documents/sto/minimal.sto");
    const std::tuple<size_t, size_t> expected = {0, 1};

    ASSERT_EQ(df.shape(), expected);
}

TEST(opynsim, read_sto_columns_returns_time_and_data_column_for_one_column_example)
{
    opyn::init();

    const DataFrame df = read_sto(opynsim_tests_resources_directory() / "Documents/sto/one_data_column.sto");
    const std::vector<std::string> expected = {"time", "column1"};

    ASSERT_EQ(df.columns(), expected);
}

TEST(opynsim, read_sto_columns_returns_expected_attrs_for_one_column_example)
{
    opyn::init();

    const DataFrame df = read_sto(opynsim_tests_resources_directory() / "Documents/sto/one_data_column.sto");
    const std::unordered_map<std::string, std::string> expected = {
        {"meta1", "a"},
    };

    ASSERT_EQ(df.attrs(), expected);
}

TEST(opynsim, read_sto_shape_returns_1_2_for_one_column_example)
{
    opyn::init();

    const DataFrame df = read_sto(opynsim_tests_resources_directory() / "Documents/sto/one_data_column.sto");
    const std::tuple<size_t, size_t> expected = {1, 2};

    ASSERT_EQ(df.shape(), expected);
}

TEST(opynsim, read_sto_columns_returns_time_and_two_data_columns_for_two_column_example)
{
    opyn::init();

    const DataFrame df = read_sto(opynsim_tests_resources_directory() / "Documents/sto/two_data_columns.sto");
    const std::vector<std::string> expected = {"time", "column1", "column2"};

    ASSERT_EQ(df.columns(), expected);
}

TEST(opynsim, read_sto_columns_returns_expected_attrs_for_two_column_example)
{
    opyn::init();

    const DataFrame df = read_sto(opynsim_tests_resources_directory() / "Documents/sto/two_data_columns.sto");
    const std::unordered_map<std::string, std::string> expected = {
        {"meta1", "a"},
        {"meta2", "b"},
    };

    ASSERT_EQ(df.attrs(), expected);
}

TEST(opynsim, read_sto_shape_returns_3_2_for_two_column_example)
{
    opyn::init();

    const DataFrame df = read_sto(opynsim_tests_resources_directory() / "Documents/sto/two_data_columns.sto");
    const std::tuple<size_t, size_t> expected = {2, 3};

    ASSERT_EQ(df.shape(), expected);
}

TEST(opynsim, read_sto_attrs_contains_header_if_non_kv_headers_are_in_file)
{
    // Note: this is to support legacy OpenSim behavior, where some STO files
    // will embed things like "Results" "Experiment 1" etc. into the header of
    // the STO file as a non-key-value string.

    opyn::init();

    const DataFrame df = read_sto(opynsim_tests_resources_directory() / "Documents/sto/has_header.sto");
    const std::unordered_map<std::string, std::string> expected = {
        {"header", "Non key-value headers can continue...\n... around key values."},
        {"attr1", "x"},
        {"attr2", "y"},
    };

    ASSERT_EQ(df.attrs(), expected);
}

TEST(opynsim, read_sto_prints_expected_pretty_string_in_two_column_case)
{
    opyn::init();

    const DataFrame df = read_sto(opynsim_tests_resources_directory() / "Documents/sto/two_data_columns.sto");
    const std::string got = osc::stream_to_string(df);
    const std::string_view expected = R"(shape: (2, 3)
| time | column1 | column2 |
|:-----|:--------|:--------|
| 0.0  | 2.0     | 3.0     |
| 1.0  | 4.0     | 6.0     |
)";
    ASSERT_EQ(got, expected);
}

TEST(opynsim, read_sto_can_read_and_print_a_bigger_pendulum_example)
{
    opyn::init();

    const DataFrame df = read_sto(opynsim_tests_resources_directory() / "Documents/sto/double_pendulum_run.sto");
    const std::string got = osc::stream_to_string(df);

    ASSERT_FALSE(got.empty());
}

TEST(opynsim, read_sto_accepts_duplicate_time_rows)
{
    // This behavior is counter to OpenSim.
    //
    // In OpenSim, an exception is thrown from `OpenSim::TimeSeriesTable` if
    // two rows of an STO file have the same timestamp. However, `OpenSim::Storage`
    // (legacy) used to allow it, which means there's STO files in the wild that
    // contain duplicate rows. OPynSim falls back to the legacy behavior so that
    // the parser is permissive enough to accept legacy data files. It's the
    // caller's responsibility to clean up the mess.

    opyn::init();

    const DataFrame df = read_sto(opynsim_tests_resources_directory() / "Documents/sto/duplicate_time_row.sto");
    const std::vector<std::string> expected_columns = {"time", "col1", "col2"};
    const std::vector<double> expected_timestamps = {0.0, 0.001, 0.001, 0.002};  // note: contains duplicates

    ASSERT_EQ(df.columns(), expected_columns);
    ASSERT_EQ(df["time"].to_list(), expected_timestamps);
}

TEST(opynsim, read_sto_flatterns_quaternion_stos)
{
    opyn::init();

    const DataFrame df = read_sto(opynsim_tests_resources_directory() / "Documents/sto/quaternion.sto");
    const std::vector<std::string> expected_columns = {"time", "pelvis_w", "pelvis_x", "pelvis_y", "pelvis_z"};
    const std::vector<double> expected_timestamps = {0.0, 1.0};
    const std::tuple<size_t, size_t> expected_shape = {2, 5};

    ASSERT_EQ(df.columns(), expected_columns);
    ASSERT_EQ(df["time"].to_list(), expected_timestamps);
    ASSERT_EQ(df.shape(), expected_shape);
}

TEST(opynsim, read_sto_permits_legacy_delimiters)
{
    // Some older/custom STO files in opensim-models and SimTK use ' '
    // as a delimiter. It's invalid, and rejected by the newer
    // `OpenSim::TimeSeriesTable` API, but the legacy
    // `OpenSim::Storage` API still accepts+parses these files, which
    // means that applications like OpenSim GUI will accept the file, so
    // OPynSim must also accept the file.

    using namespace osc;  // `begin`, `end`

    opyn::init();

    const DataFrame df = read_sto(opynsim_tests_resources_directory() / "Documents/sto/legacy_delimiters.sto");
    std::vector<std::string> expected_column_labels = {"time", "col1"};
    std::vector<double> expected_times = {0.654092, 0.970842, 0.981167};
    std::vector<double> expected_values = {0.000000, 0.256537, 0.256537};

    ASSERT_EQ(df.columns(), expected_column_labels);
    ASSERT_EQ(df["time"].to_list(), expected_times);
    ASSERT_EQ(df["col1"].to_list(), expected_values);
}

TEST(opynsim, read_mot_works_on_minimal_example)
{
    opyn::init();

    ASSERT_NO_THROW({ read_mot(opynsim_tests_resources_directory() / "Documents/minimal.mot"); });
}

TEST(opynsim, read_mot_columns_returns_time_column_for_minimal_example)
{
    opyn::init();

    const DataFrame df = read_mot(opynsim_tests_resources_directory() / "Documents/minimal.mot");
    const std::vector<std::string> expected = {"time"};

    ASSERT_EQ(df.columns(), expected);
}

TEST(opynsim, read_mot_parses_one_data_column_correctly)
{
    opyn::init();

    const DataFrame df = read_mot(opynsim_tests_resources_directory() / "Documents/one_data_column.mot");
    const std::vector<std::string> expected_columns = {"time", "column1"};
    const std::unordered_map<std::string, std::string> expected_attrs = {{"header", "meta isn't a thing in OpenSim's mot file parser?"}};
    const std::tuple<size_t, size_t> expected_shape = {1, 2};

    ASSERT_EQ(df.columns(), expected_columns);
    ASSERT_EQ(df.attrs(), expected_attrs);
    ASSERT_EQ(df.shape(), expected_shape);
}

TEST(opynsim, read_mot_accepts_duplicate_time_rows)
{
    // This behavior is counter to OpenSim.
    //
    // In OpenSim, an exception is thrown from `OpenSim::TimeSeriesTable` if
    // two rows of a MOT file have the same timestamp. OPynSim falls back
    // to the permissive behavior so that  the parser can accept legacy
    // data files. It's the caller's responsibility to clean up the mess.

    opyn::init();

    const DataFrame df = read_mot(opynsim_tests_resources_directory() / "Documents/duplicate_time_row.mot");
    const std::vector<std::string> expected_columns = {"time", "col1", "col2"};
    const std::vector<double> expected_timestamps = {0.0, 0.001, 0.001, 0.002};  // note: contains duplicates

    ASSERT_EQ(df.columns(), expected_columns);
    ASSERT_EQ(df["time"].to_list(), expected_timestamps);
}

TEST(opynsim, read_trc_works_on_minimal_example)
{
    opyn::init();

    ASSERT_NO_THROW({ read_trc(opynsim_tests_resources_directory() / "Documents/minimal.trc"); });
}

TEST(opynsim, read_trc_prints_expected_pretty_printed_string_for_minimal_example)
{
    opyn::init();

    const DataFrame df = read_trc(opynsim_tests_resources_directory() / "Documents/minimal.trc");
    const std::string got = osc::stream_to_string(df);
    const std::string_view expected = R"(shape: (1, 4)
| time | Marker1_x | Marker1_y | Marker1_z |
|:-----|:----------|:----------|:----------|
| 1.0  | 1.0       | 2.0       | 3.0       |
)";
    ASSERT_EQ(got, expected);
}

TEST(opynsim, read_trc_parses_leg39_swing_short_trc_correctly)
{
    opyn::init();

    const DataFrame df = read_trc(opynsim_tests_resources_directory() / "Documents/leg39_swing_short.trc");
    const auto raw_column_names = std::to_array<std::string_view>({
        "R.ASIS",
        "L.ASIS",
        "V.Sacral",
        "R.Thigh.Upper",
        "R.Thigh.Front",
        "R.Thigh.Rear",
        "L.Thigh.Upper",
        "L.Thigh.Front",
        "L.Thigh.Rear",
        "R.Shank.Upper",
        "R.Shank.Front",
        "R.Shank.Rear",
        "L.Shank.Upper",
        "L.Shank.Front",
        "L.Shank.Rear",
        "R.Heel",
        "R.Midfoot.Sup",
        "R.Midfoot.Lat",
        "R.Toe.Tip",
        "L.Heel",
        "L.Midfoot.Sup",
        "L.Midfoot.Lat",
        "L.Toe.Tip",
        "Sternum",
        "R.Acromium",
        "L.Acromium",
        "R.Bicep",
        "L.Bicep",
        "R.Elbow",
        "L.Elbow",
        "R.Wrist.Med",
        "R.Wrist.Lat",
        "L.Wrist.Med",
        "L.Wrist.Lat",
        "R.Toe.Lat",
        "R.Toe.Med",
        "L.Toe.Lat",
        "L.Toe.Med",
        "R.Temple",
        "L.Temple",
        "Top.Head",
    });
    std::vector<std::string> expected_column_names;
    expected_column_names.reserve(1 + 3*raw_column_names.size());
    expected_column_names.emplace_back("time");
    for (const auto& raw_column_name : raw_column_names) {
        expected_column_names.push_back(std::string{raw_column_name} + "_x");
        expected_column_names.push_back(std::string{raw_column_name} + "_y");
        expected_column_names.push_back(std::string{raw_column_name} + "_z");
    }

    ASSERT_EQ(df.size(), expected_column_names.size());
    ASSERT_EQ(df.columns(), expected_column_names);
    ASSERT_EQ(df.height(), 14);
    ASSERT_EQ(df.width(), expected_column_names.size());
}

TEST(opynsim, read_trc_accepts_duplicate_time_rows)
{
    // This behavior is counter to OpenSim.
    //
    // In OpenSim, an exception is thrown from `OpenSim::TimeSeriesTable` if
    // two rows of a TRC file have the same timestamp. OPynSim falls back
    // to the permissive behavior so that  the parser can accept legacy
    // data files. It's the caller's responsibility to clean up the mess.

    opyn::init();

    const DataFrame df = read_trc(opynsim_tests_resources_directory() / "Documents/duplicate_time_row.trc");
    const std::vector<std::string> expected_columns = {"time", "Marker1_x", "Marker1_y", "Marker1_z"};
    const std::vector<double> expected_timestamps = {0.0, 1.0, 1.0, 2.0};  // note: contains duplicates

    ASSERT_EQ(df.columns(), expected_columns);
    ASSERT_EQ(df["time"].to_list(), expected_timestamps);
}

TEST(opynsim, read_csv_works_on_minimal_example)
{
    opyn::init();

    ASSERT_NO_THROW({ read_csv(opynsim_tests_resources_directory() / "Documents/minimal.csv"); });
}

TEST(opynsim, read_csv_prints_expected_pretty_printed_string_for_minimal_example)
{
    opyn::init();

    const DataFrame df = read_csv(opynsim_tests_resources_directory() / "Documents/minimal.csv");
    const std::string got = osc::stream_to_string(df);
    const std::string_view expected = R"(shape: (1, 2)
| time | header1 |
|:-----|:--------|
| 0.0  | 1.0     |
)";

    ASSERT_EQ(got, expected);
}

TEST(opynsim, read_csv_works_for_two_row_example)
{
    opyn::init();

    const DataFrame df = read_csv(opynsim_tests_resources_directory() / "Documents/two_rows.csv");
    const std::vector<std::string> expected_columns = {"time", "x", "y", "z"};
    const std::vector<double> expected_times = {1.0, 2.0};

    ASSERT_EQ(df.columns(), expected_columns);
    ASSERT_EQ(df.width(), 4);
    ASSERT_EQ(df.height(), 2);
    ASSERT_EQ(df.shape(), std::tuple(2uz, 4uz));
    ASSERT_EQ(df["time"].to_list(), expected_times);
}

TEST(opynsim, read_vtp_works_for_minimal_example)
{
    opyn::init();

    const osc::Mesh mesh = read_vtp(opynsim_tests_resources_directory() / "Documents/minimal.vtp");

    ASSERT_EQ(mesh.num_vertices(), 1);
    ASSERT_EQ(mesh.num_indices(), 0);
    ASSERT_EQ(mesh.topology(), osc::MeshTopology::Triangles);
}

TEST(opynsim, read_vtp_works_for_triangle)
{
    opyn::init();

    const osc::Mesh mesh = read_vtp(opynsim_tests_resources_directory() / "Documents/triangle.vtp");
    const std::vector<osc::Vector3f> expected_vertices = {
        {-1.0f, -1.0f, 0.0f},
        { 1.0f, -1.0f, 0.0f},
        { 0.0f,  1.0f, 0.0f},
    };

    ASSERT_EQ(mesh.num_vertices(), 3);
    ASSERT_EQ(mesh.num_indices(), 3);
    ASSERT_EQ(mesh.topology(), osc::MeshTopology::Triangles);
    ASSERT_EQ(mesh.vertices(), expected_vertices);
}

TEST(opynsim, read_obj_works_for_minimal_example)
{
    opyn::init();

    ASSERT_NO_THROW({ read_obj(opynsim_tests_resources_directory() / "Documents/minimal.obj"); });
}

TEST(opynsim, read_obj_works_for_triangle)
{
    opyn::init();

    const osc::Mesh mesh = read_obj(opynsim_tests_resources_directory() / "Documents/triangle.obj");
    const std::vector<osc::Vector3f> expected_vertices = {
        {-1.0f, -1.0f, 0.0f},
        { 1.0f, -1.0f, 0.0f},
        { 0.0f,  1.0f, 0.0f},
    };

    ASSERT_EQ(mesh.num_vertices(), expected_vertices.size());
    ASSERT_EQ(mesh.num_indices(), 3);
    ASSERT_EQ(mesh.topology(), osc::MeshTopology::Triangles);
    ASSERT_EQ(mesh.vertices(), expected_vertices);
}

TEST(opynsim, read_stl_works_for_minimal_example)
{
    opyn::init();

    ASSERT_NO_THROW({ read_stl(opynsim_tests_resources_directory() / "Documents/minimal.stl"); });
}

TEST(opynsim, read_stl_works_for_a_triangle)
{
    opyn::init();

    const osc::Mesh mesh = read_stl(opynsim_tests_resources_directory() / "Documents/triangle.stl");
    const std::vector<osc::Vector3f> expected_vertices = {
        {-1.0f, -1.0f, 0.0f},
        { 1.0f, -1.0f, 0.0f},
        { 0.0f,  1.0f, 0.0f},
    };

    ASSERT_EQ(mesh.num_vertices(), expected_vertices.size());
    ASSERT_EQ(mesh.num_indices(), 3);
    ASSERT_EQ(mesh.topology(), osc::MeshTopology::Triangles);
    ASSERT_EQ(mesh.vertices(), expected_vertices);
}

TEST(opynsim, read_png_works_for_minimal_png_file)
{
    opyn::init();

    const osc::Texture2D png = read_png(opynsim_tests_resources_directory() / "Documents/minimal.png");

    ASSERT_EQ(png.pixel_dimensions(), osc::Vector2i(1, 1));
    ASSERT_EQ(png.texture_format(), osc::TextureFormat::RGBA32);
    ASSERT_EQ(png.pixels().front(), osc::Color::white());
}

TEST(opynsim, read_jpeg_works_for_minimal_jpeg_file)
{
    opyn::init();

    const osc::Texture2D jpeg = read_png(opynsim_tests_resources_directory() / "Documents/minimal.jpeg");

    ASSERT_EQ(jpeg.pixel_dimensions(), osc::Vector2i(1, 1));
    ASSERT_EQ(jpeg.texture_format(), osc::TextureFormat::RGB24);
    ASSERT_EQ(jpeg.pixels().front(), osc::Color::white());
}

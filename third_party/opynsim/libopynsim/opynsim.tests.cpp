#include "opynsim.h"

#include <libopynsim/tests/opynsim_tests_config.h>
#include <libopynsim/model_specification.h>

#include <gtest/gtest.h>

#include <string>
#include <unordered_map>
#include <utility>
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
    const std::pair<size_t, size_t> expected = {0, 1};

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
    const std::pair<size_t, size_t> expected = {1, 2};

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
    const std::pair<size_t, size_t> expected = {2, 3};

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

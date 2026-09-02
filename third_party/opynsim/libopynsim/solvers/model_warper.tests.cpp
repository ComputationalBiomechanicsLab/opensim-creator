#include "model_warper.h"

#include <libopynsim/tests/opynsim_tests_config.h>
#include <libopynsim/model_specification.h>
#include <libopynsim/model.h>
#include <libopynsim/opynsim.h>

#include <gtest/gtest.h>
#include <liboscar/utilities/temporary_directory.h>

#include <filesystem>

using namespace opyn;

TEST(ModelWarper, can_default_construct)
{
    opyn::init();

    const ModelWarper model_warper;
    ASSERT_EQ(model_warper.num_scaling_steps(), 0);
    ASSERT_EQ(model_warper.num_scaling_parameters(), 0);
}

TEST(ModelWarper, can_load_scaling_document_written_by_opensimcreator)
{
    // This is a sanity check, and somewhat redundant with `ModelWarperV3Document`'s
    // test suite, but should pass because OPynSim's Python API should be able to
    // load files from OpenSim Creator.

    opyn::init();

    const ModelWarper model_warper{opynsim_tests_resources_directory() / "Documents/model_warper/scaling-document.xml"};
    ASSERT_EQ(model_warper.num_scaling_steps(), 6);
    ASSERT_EQ(model_warper.num_scaling_parameters(), 1);
}

TEST(ModelWarper, can_warp_example_model_from_opensimcreator)
{
    // This automatically checks that the model + scaling document pair published
    // in OpenSim Creator's documentation when the model warper was first developed
    // still works and warps the model to disk, which is a very high-level way
    // of performing the warp.

    namespace fs = std::filesystem;

    opyn::init();

    // Load input data, create a warper for it
    const ModelSpecification model_specification = read_osim(opynsim_tests_resources_directory() / "Documents/model_warper/make-a-leg.osim");
    const ModelWarper model_warper{opynsim_tests_resources_directory() / "Documents/model_warper/scaling-document.xml"};

    // Configure output directory and warp to it.
    const osc::TemporaryDirectory output_directory;
    const fs::path output_model_path = output_directory.absolute_path() / "warped_model.osim";
    const fs::path warped_geometry_directory_name = "some_warped_geom";
    const fs::path expected_warped_geometry_path = output_model_path.parent_path() / warped_geometry_directory_name;
    model_warper.warp_to_osim_file(
        model_specification,
        output_model_path,
        warped_geometry_directory_name
    );

    // Check outputs
    ASSERT_TRUE(fs::exists(output_model_path) and fs::is_regular_file(output_model_path));
    ASSERT_TRUE(fs::exists(expected_warped_geometry_path) and fs::is_directory(expected_warped_geometry_path));
    const ModelSpecification warped_specification = read_osim(output_model_path);

    const Model input_model = model_specification.compile();
    const Model warped_model = warped_specification.compile();

    ASSERT_EQ(input_model.coordinates(), warped_model.coordinates());
    ASSERT_EQ(input_model.outputs(), warped_model.outputs());
}

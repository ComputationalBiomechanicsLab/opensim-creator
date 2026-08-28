#include "model_warper_v3_document.h"

#include <libopynsim/tests/opynsim_tests_config.h>

#include <libopynsim/solvers/model_warper/thin_plate_spline_meshes_scaling_step.h>
#include <libopynsim/solvers/model_warper/thin_plate_spline_stations_scaling_step.h>
#include <libopynsim/opynsim.h>

#include <gtest/gtest.h>

using namespace opyn;

TEST(ModelWarperV3Document, default_constructs_with_no_scaling_steps_or_parameters)
{
    opyn::init();

    ModelWarperV3Document model_warper_v3_document;
    ASSERT_FALSE(model_warper_v3_document.hasScalingSteps());
    ASSERT_FALSE(model_warper_v3_document.hasScalingParameters());
}

TEST(ModelWarperV3Document, can_load_scaling_document_written_by_opensimcreator)
{
    // Basic test that the `std::filesystem::path` overload is capable of loading
    // a scaling configuration written by OpenSim Creator.
    //
    // Important because many users will create the scaling document in the OpenSim Creator
    // UI and then use OPynSim to automate the process.

    opyn::init();

    ModelWarperV3Document model_warper_v3_document{opynsim_tests_resources_directory() / "Documents/model_warper_scaling-document-from-docs.xml"};
    ASSERT_TRUE(model_warper_v3_document.hasScalingSteps());
    ASSERT_TRUE(model_warper_v3_document.hasScalingParameters());
    ASSERT_NE(dynamic_cast<const ThinPlateSplineMeshesScalingStep*>(model_warper_v3_document.findComponent("thinplatesplinemeshesscalingstep")), nullptr);
    ASSERT_NE(dynamic_cast<const ThinPlateSplineStationsScalingStep*>(model_warper_v3_document.findComponent("thinplatesplinestationsscalingstep")), nullptr);
}
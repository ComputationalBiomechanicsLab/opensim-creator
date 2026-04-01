#include "model_specification.h"

#include <libopynsim/tests/opynsim_tests_config.h>
#include <libopynsim/opynsim.h>

#include <gtest/gtest.h>

using namespace opyn;

TEST(ModelSpecification, compile_works_on_blank_ModelSpecification)
{
    opyn::init();

    const ModelSpecification model_specification;
    ASSERT_NO_THROW({ model_specification.compile(); });
}

TEST(ModelSpecification, compile_works_on_more_complicated_example_OpenSim_model)
{
    opyn::init();

    const ModelSpecification model_specification = import_osim_file(opynsim_tests_resources_directory() / "models/RajagopalModel/Rajagopal2015.osim");
    ASSERT_NO_THROW({ model_specification.compile(); });
}

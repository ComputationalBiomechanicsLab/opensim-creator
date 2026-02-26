#include "opynsim.h"

#include <libopynsim/tests/opynsim_tests_config.h>
#include <libopynsim/model_specification.h>

#include <gtest/gtest.h>

using namespace opyn;

TEST(opynsim, import_osim_file_throws_if_file_doesnt_exist)
{
    opyn::init();

    ASSERT_ANY_THROW({ import_osim_file("/this/probably/doesnt/exist"); });
}

TEST(opynsim, import_osim_file_works_when_given_a_file_that_does_exist)
{
    opyn::init();

    ASSERT_NO_THROW({ import_osim_file(opynsim_tests_resources_directory() / "models/Blank/blank.osim"); });
}

TEST(opynsim, compile_specification_works_on_blank_ModelSpecification)
{
    opyn::init();

    const ModelSpecification model_specification;
    ASSERT_NO_THROW({ compile_specification(model_specification); });
}

TEST(opynsim, compile_specification_works_on_more_complicated_example_OpenSim_model)
{
    opyn::init();

    const ModelSpecification model_specification = import_osim_file(opynsim_tests_resources_directory() / "models/RajagopalModel/Rajagopal2015.osim");
    ASSERT_NO_THROW({ compile_specification(model_specification); });
}

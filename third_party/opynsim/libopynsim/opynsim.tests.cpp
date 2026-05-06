#include "opynsim.h"

#include <libopynsim/tests/opynsim_tests_config.h>
#include <libopynsim/model_specification.h>

#include <gtest/gtest.h>

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

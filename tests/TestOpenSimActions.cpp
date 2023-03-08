#include "osc_test_config.hpp"

#include "src/OpenSimBindings/ActionFunctions.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"

#include <gtest/gtest.h>
#include <OpenSim/Simulation/Model/Model.h>

// repro for #642
//
// @AdrianHendrik reported that trying to add a body with an invalid name entirely crashes
// OSC, which implies that the operation causes a segfault
TEST(OpenSimActions, ActionAddBodyToModelThrowsIfBodyNameIsInvalid)
{
    osc::UndoableModelStatePair model;

    osc::BodyDetails details;
    details.bodyName = "test 1";
    details.parentFrameAbsPath = "/ground";  // this is what the dialog defaults to

    ASSERT_ANY_THROW({ osc::ActionAddBodyToModel(model, details); });
}
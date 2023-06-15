#include "OpenSimCreator/ActionFunctions.hpp"

#include "OpenSimCreator/Model/UndoableModelStatePair.hpp"
#include "testopensimcreator_config.hpp"

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

// repro for #495
//
// @JuliaVanBeesel reported that, when editing an OpenSim model via the editor UI, if
// they then delete the backing file (e.g. via Windows explorer), the editor UI will
// then show an error message from an exception, rather than carrying on or warning
// that something not-quite-right has happened
TEST(OpenSimActions, ActionUpdateModelFromBackingFileReturnsFalseIfFileDoesNotExist)
{
    osc::UndoableModelStatePair model;

    // it just returns `false` if there's no backing file
    ASSERT_FALSE(osc::ActionUpdateModelFromBackingFile(model));

    // ... but if you say it has an invalid backing file path...
    model.setFilesystemPath("doesnt-exist");

    // then it should just return `false`, rather than (e.g.) exploding
    ASSERT_FALSE(osc::ActionUpdateModelFromBackingFile(model));
}

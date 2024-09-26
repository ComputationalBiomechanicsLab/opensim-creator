#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>

#include <TestOpenSimCreator/TestOpenSimCreatorConfig.h>

#include <gtest/gtest.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSimCreator/Graphics/OpenSimDecorationGenerator.h>
#include <OpenSimCreator/Graphics/OpenSimDecorationOptions.h>
#include <OpenSimCreator/Platform/OpenSimCreatorApp.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>
#include <oscar/Formats/DAE.h>
#include <oscar/Graphics/Scene/SceneCache.h>
#include <oscar/Graphics/Scene/SceneDecoration.h>
#include <oscar/Utils/NullOStream.h>

#include <filesystem>
#include <functional>
#include <sstream>

using namespace osc;

TEST(UndoableModelStatePair, CanLoadAndRenderAllUserFacingExampleFiles)
{
    // ensure the OpenSim API is initialized and the meshes are loadable from
    // the central `geometry/` directory
    {
        GloballyInitOpenSim();
        GloballyAddDirectoryToOpenSimGeometrySearchPath(std::filesystem::path{OSC_RESOURCES_DIR} / "geometry");
    }

    SceneCache meshCache;

    // turn as many decoration options on as possible, so that the code gets tested
    // against them (#661)
    OpenSimDecorationOptions decorationOpts;
    decorationOpts.setShouldShowAnatomicalMuscleLineOfActionForInsertion(true);
    decorationOpts.setShouldShowAnatomicalMuscleLineOfActionForOrigin(true);
    decorationOpts.setShouldShowEffectiveMuscleLineOfActionForInsertion(true);
    decorationOpts.setShouldShowEffectiveMuscleLineOfActionForOrigin(true);
    decorationOpts.setShouldShowCentersOfMass(true);
    decorationOpts.setShouldShowScapulo(true);
    decorationOpts.setShouldShowPointToPointSprings(true);

    const std::filesystem::path examplesDir = std::filesystem::path{OSC_RESOURCES_DIR} / "models";
    ASSERT_TRUE(std::filesystem::exists(examplesDir) && std::filesystem::is_directory(examplesDir));

    size_t nExamplesTested = 0;
    for (const std::filesystem::directory_entry& e : std::filesystem::recursive_directory_iterator{examplesDir})
    {
        if (e.is_regular_file() && e.path().extension() == ".osim")
        {
            // all files are loadable
            UndoableModelStatePair p{e.path()};

            // and all can be used to generate 3D scenes
            std::vector<SceneDecoration> decorations;
            GenerateModelDecorations(
                meshCache,
                p.getModel(),
                p.getState(),
                decorationOpts,
                1.0f,  // 1:1 scaling
                [&decorations](const OpenSim::Component& component, SceneDecoration&& dec)
                {
                    dec.id = GetAbsolutePathStringName(component);
                    decorations.push_back(std::move(dec));
                }
            );

            // and decorations are generated
            ASSERT_FALSE(decorations.empty());

            // and all decorations can be exported to a DAE format
            NullOStream stream;
            const DAEMetadata metadata{TESTOPENSIMCREATOR_APPNAME_STRING, TESTOPENSIMCREATOR_APPNAME_STRING};
            write_as_dae(stream, decorations, metadata);

            // and content is actually written to the DAE stream
            ASSERT_TRUE(stream.was_written_to());

            ++nExamplesTested;
        }
    }
    ASSERT_GT(nExamplesTested, 10);  // sanity check: remove this if you want <10 examples
}

// related issue: #890
//
// calling `setModel` with an `OpenSim::Model` should retain the scene scale factor of
// the current scratch space
TEST(UndoableModelStatePair, setModelRetainsSceneScaleFactor)
{
    UndoableModelStatePair model;

    ASSERT_EQ(model.getFixupScaleFactor(), 1.0f);
    model.setFixupScaleFactor(0.5f);
    ASSERT_EQ(model.getFixupScaleFactor(), 0.5f);

    model.setModel(std::make_unique<OpenSim::Model>());
    ASSERT_EQ(model.getFixupScaleFactor(), 0.5f);
}

TEST(UndoableModelStatePair, resetModelRetainsSceneScaleFactor)
{
    UndoableModelStatePair model;

    ASSERT_EQ(model.getFixupScaleFactor(), 1.0f);
    model.setFixupScaleFactor(0.5f);
    ASSERT_EQ(model.getFixupScaleFactor(), 0.5f);

    model.resetModel();
    ASSERT_EQ(model.getFixupScaleFactor(), 0.5f);
}
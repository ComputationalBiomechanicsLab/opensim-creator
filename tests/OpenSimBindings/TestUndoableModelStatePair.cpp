#include "src/OpenSimBindings/UndoableModelStatePair.hpp"

#include "src/Graphics/MeshCache.hpp"
#include "src/OpenSimBindings/Graphics/CustomDecorationOptions.hpp"
#include "src/OpenSimBindings/Graphics/OpenSimDecorationGenerator.hpp"
#include "osc_test_config.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <functional>

TEST(UndoableModelStatePair, CanLoadAndRenderAllUserFacingExampleFiles)
{
    osc::MeshCache meshCache;

    // turn as many decoration options on as possible, so that the code gets tested
    // against them (#661)
    osc::CustomDecorationOptions decorationOpts;
    decorationOpts.setShouldShowAnatomicalMuscleLineOfActionForInsertion(true);
    decorationOpts.setShouldShowAnatomicalMuscleLineOfActionForOrigin(true);
    decorationOpts.setShouldShowEffectiveMuscleLineOfActionForInsertion(true);
    decorationOpts.setShouldShowEffectiveMuscleLineOfActionForOrigin(true);
    decorationOpts.setShouldShowCentersOfMass(true);
    decorationOpts.setShouldShowScapulo(true);
    decorationOpts.setShouldShowPointToPointSprings(true);

    std::filesystem::path const examplesDir = std::filesystem::path{OSC_TESTING_SOURCE_DIR} / "resources" / "models";
    ASSERT_TRUE(std::filesystem::exists(examplesDir) && std::filesystem::is_directory(examplesDir));

    int nExamples = 0;
    for (std::filesystem::directory_entry const& e : std::filesystem::recursive_directory_iterator{examplesDir})
    {
        if (e.is_regular_file() && e.path().extension() == ".osim")
        {
            // all files are loadable
            osc::UndoableModelStatePair p{e.path()};

            // and all can be used to generate 3D scenes
            bool containsDecorations = false;
            osc::GenerateModelDecorations(
                meshCache,
                p.getModel(),
                p.getState(),
                decorationOpts,
                1.0f,  // 1:1 scaling
                [&containsDecorations](OpenSim::Component const&, osc::SceneDecoration&&)
                {
                    containsDecorations = true;
                }
            );

            ++nExamples;
        }
    }

    ASSERT_GT(nExamples, 10);  // sanity check: remove this if you want <10 examples
}

#include "OpenSimCreator/UndoableModelStatePair.hpp"

#include "OpenSimCreator/Graphics/OpenSimDecorationGenerator.hpp"
#include "OpenSimCreator/Graphics/OpenSimDecorationOptions.hpp"
#include "OpenSimCreator/Utils/OpenSimHelpers.hpp"
#include "OpenSimCreator/OpenSimApp.hpp"
#include "testopensimcreator_config.hpp"

#include <oscar/Formats/DAE.hpp>
#include <oscar/Graphics/MeshCache.hpp>
#include <oscar/Graphics/SceneDecoration.hpp>
#include <oscar/Platform/Config.hpp>

#include <gtest/gtest.h>
#include <OpenSim/Common/Component.h>

#include <sstream>
#include <filesystem>
#include <functional>

TEST(UndoableModelStatePair, CanLoadAndRenderAllUserFacingExampleFiles)
{
    osc::GlobalInitOpenSim(*osc::Config::load());

    osc::MeshCache meshCache;

    // turn as many decoration options on as possible, so that the code gets tested
    // against them (#661)
    osc::OpenSimDecorationOptions decorationOpts;
    decorationOpts.setShouldShowAnatomicalMuscleLineOfActionForInsertion(true);
    decorationOpts.setShouldShowAnatomicalMuscleLineOfActionForOrigin(true);
    decorationOpts.setShouldShowEffectiveMuscleLineOfActionForInsertion(true);
    decorationOpts.setShouldShowEffectiveMuscleLineOfActionForOrigin(true);
    decorationOpts.setShouldShowCentersOfMass(true);
    decorationOpts.setShouldShowScapulo(true);
    decorationOpts.setShouldShowPointToPointSprings(true);

    std::filesystem::path const examplesDir = std::filesystem::path{OSC_TESTING_SOURCE_DIR} / "resources" / "models";
    ASSERT_TRUE(std::filesystem::exists(examplesDir) && std::filesystem::is_directory(examplesDir));

    size_t nExamplesTested = 0;
    for (std::filesystem::directory_entry const& e : std::filesystem::recursive_directory_iterator{examplesDir})
    {
        if (e.is_regular_file() && e.path().extension() == ".osim")
        {
            // all files are loadable
            osc::UndoableModelStatePair p{e.path()};

            // and all can be used to generate 3D scenes
            std::vector<osc::SceneDecoration> decorations;
            osc::GenerateModelDecorations(
                meshCache,
                p.getModel(),
                p.getState(),
                decorationOpts,
                1.0f,  // 1:1 scaling
                [&decorations](OpenSim::Component const& component, osc::SceneDecoration&& dec)
                {
                    dec.id = osc::GetAbsolutePathString(component);
                    decorations.push_back(std::move(dec));
                }
            );

            // and decorations are generated
            ASSERT_FALSE(decorations.empty());

            // and all decorations can be exported to a DAE format
            std::stringstream daeData;
            osc::WriteDecorationsAsDAE(decorations, daeData);

            // and content is actually written to the DAE stream
            ASSERT_FALSE(daeData.str().empty());

            ++nExamplesTested;
        }
    }
    ASSERT_GT(nExamplesTested, 0);  // sanity check: remove this if you want <10 examples
}

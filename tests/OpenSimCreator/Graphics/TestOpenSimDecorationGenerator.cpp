#include "OpenSimCreator/Graphics/OpenSimDecorationGenerator.hpp"

#include "OpenSimCreator/Graphics/OpenSimDecorationOptions.hpp"
#include "OpenSimCreator/Graphics/MuscleColoringStyle.hpp"
#include "OpenSimCreator/Utils/OpenSimHelpers.hpp"
#include "testopensimcreator_config.hpp"

#include <oscar/Graphics/MeshCache.hpp>
#include <oscar/Graphics/SceneDecoration.hpp>
#include <oscar/Utils/StringHelpers.hpp>
#include <oscar/Platform/Log.hpp>

#include <gtest/gtest.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <filesystem>
#include <utility>
#include <vector>

// test that telling OSC to generate OpenSim-colored muscles
// results in red muscle lines (as opposed to muscle lines that
// are based on something like excitation - #663)
TEST(OpenSimDecorationGenerator, GenerateDecorationsWithOpenSimMuscleColoringGeneratesRedMuscles)
{
    // TODO: this should be more synthetic and should just create a body with one muscle with a
    // known color that is then pumped through the pipeline etc.
    std::filesystem::path const tugOfWarPath = std::filesystem::path{OSC_TESTING_SOURCE_DIR} / "resources" / "models" / "Tug_of_War" / "Tug_of_War.osim";
    OpenSim::Model model{tugOfWarPath.string()};
    model.buildSystem();
    SimTK::State& state = model.initializeState();

    osc::OpenSimDecorationOptions opts;
    opts.setMuscleColoringStyle(osc::MuscleColoringStyle::OpenSimAppearanceProperty);

    osc::MeshCache meshCache;
    bool passedTest = false;
    osc::GenerateModelDecorations(
        meshCache,
        model,
        state,
        opts,
        1.0f,
        [&passedTest](OpenSim::Component const& c, osc::SceneDecoration&& dec)
        {
            if (osc::ContainsSubstringCaseInsensitive(c.getName(), "muscle1"))
            {
                // check that it's red
                ASSERT_GT(dec.color.r, 0.5f);
                ASSERT_GT(dec.color.r, 5.0f*dec.color.g);
                ASSERT_GT(dec.color.r, 5.0f*dec.color.b);
                passedTest = true;
            }
        }
    );
    ASSERT_TRUE(passedTest);
}

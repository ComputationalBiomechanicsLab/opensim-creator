#include <OpenSimCreator/Graphics/OpenSimDecorationGenerator.h>

#include <TestOpenSimCreator/TestOpenSimCreatorConfig.h>

#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSimCreator/Graphics/MuscleColoringStyle.h>
#include <OpenSimCreator/Graphics/OpenSimDecorationOptions.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>
#include <gtest/gtest.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Platform/Log.h>
#include <oscar/Graphics/Scene/SceneCache.h>
#include <oscar/Graphics/Scene/SceneDecoration.h>
#include <oscar/Utils/StringHelpers.h>

#include <filesystem>
#include <utility>
#include <vector>

using namespace osc;

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

    OpenSimDecorationOptions opts;
    opts.setMuscleColoringStyle(MuscleColoringStyle::OpenSimAppearanceProperty);

    SceneCache meshCache;
    bool passedTest = false;
    GenerateModelDecorations(
        meshCache,
        model,
        state,
        opts,
        1.0f,
        [&passedTest](OpenSim::Component const& c, SceneDecoration&& dec)
        {
            if (ContainsCaseInsensitive(c.getName(), "muscle1"))
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


// repro for #461
//
// the bug is that the scene scale factor is blindly applied to all scene geometry
//
// this is a basic test that ensures that the scale factor argument is applied to
// non-sized scene elements (specifically, here, the ground frame geometry), rather
// than exercising the bug (seperate test)
TEST(OpenSimDecorationGenerator, GenerateDecorationsWithScaleFactorScalesFrames)
{
    OpenSim::Model model;
    model.updDisplayHints().set_show_frames(true);  // it should scale frame geometry
    model.buildSystem();
    SimTK::State const& state = model.initializeState();

    auto const generateDecorationsWithScaleFactor = [&model, &state](float scaleFactor)
    {
        SceneCache meshCache;

        std::vector<SceneDecoration> rv;
        GenerateModelDecorations(
            meshCache,
            model,
            state,
            OpenSimDecorationOptions{},
            scaleFactor,
            [&rv](OpenSim::Component const& c, SceneDecoration&& dec)
            {
                // only suck up the frame decorations associated with ground
                if (dynamic_cast<OpenSim::Ground const*>(&c))
                {
                    rv.push_back(std::move(dec));
                }
            }
        );
        return rv;
    };

    float const scale = 0.25f;
    std::vector<SceneDecoration> const unscaledDecs = generateDecorationsWithScaleFactor(1.0f);
    std::vector<SceneDecoration> const scaledDecs = generateDecorationsWithScaleFactor(scale);

    ASSERT_FALSE(unscaledDecs.empty());
    ASSERT_FALSE(scaledDecs.empty());
    ASSERT_EQ(unscaledDecs.size(), scaledDecs.size());

    for (size_t i = 0; i < unscaledDecs.size(); ++i)
    {
        SceneDecoration const& unscaledDec = unscaledDecs[i];
        SceneDecoration const& scaledDec = scaledDecs[i];

        ASSERT_TRUE(all_of(equal_within_reldiff(scale*unscaledDec.transform.scale, scaledDec.transform.scale, 0.0001f)));
    }
}

// repro for #461
//
// the bug is that the scene scale factor is blindly applied to all scene geometry
//
// this repro adds a sphere into the scene and checks that the decoration genenerator ignores
// the geometry in this particular case
TEST(OpenSimDecorationGenerator, GenerateDecorationsWithScaleFactorDoesNotScaleExplicitlyAddedSphereGeometry)
{
    // create appropriate model
    std::pair<OpenSim::Model, OpenSim::ComponentPath> p = []()
    {
        OpenSim::Model m;
        auto body = std::make_unique<OpenSim::Body>("body", 1.0, SimTK::Vec3{}, SimTK::Inertia{1.0});
        auto geom = std::make_unique<OpenSim::Sphere>(1.0);
        OpenSim::Geometry const* geomPtr = geom.get();

        body->attachGeometry(geom.release());
        m.addBody(body.release());
        m.buildSystem();

        return std::pair{std::move(m), geomPtr->getAbsolutePath()};
    }();
    p.first.buildSystem();
    SimTK::State const& state = p.first.initializeState();

    // helper
    auto const generateDecorationsWithScaleFactor = [&p, &state](float scaleFactor)
    {
        SceneCache meshCache;

        std::vector<SceneDecoration> rv;
        GenerateModelDecorations(
            meshCache,
            p.first,
            state,
            OpenSimDecorationOptions{},
            scaleFactor,
            [&p, &rv](OpenSim::Component const& c, SceneDecoration&& dec)
            {
                if (c.getAbsolutePath() == p.second)
                {
                    rv.push_back(std::move(dec));
                }
            }
        );
        return rv;
    };

    float const scale = 0.25f;
    std::vector<SceneDecoration> const unscaledDecs = generateDecorationsWithScaleFactor(1.0f);
    std::vector<SceneDecoration> const scaledDecs = generateDecorationsWithScaleFactor(scale);

    ASSERT_FALSE(unscaledDecs.empty());
    ASSERT_FALSE(scaledDecs.empty());
    ASSERT_EQ(unscaledDecs.size(), scaledDecs.size());

    for (size_t i = 0; i < unscaledDecs.size(); ++i)
    {
        SceneDecoration const& unscaledDec = unscaledDecs[i];
        SceneDecoration const& scaledDec = scaledDecs[i];

        // note: not scaled
        ASSERT_TRUE(all_of(equal_within_reldiff(unscaledDec.transform.scale, scaledDec.transform.scale, 0.0001f)));
    }
}

TEST(OpenSimDecorationGenerator, ToOscMeshWorksAsIntended)
{
    std::filesystem::path const arrowPath = std::filesystem::path{OSC_TESTING_SOURCE_DIR} / "build_resources" / "TestOpenSimCreator" / "arrow.vtp";

    OpenSim::Model model;
    auto& mesh = AddComponent(model, std::make_unique<OpenSim::Mesh>(arrowPath.string()));
    mesh.setFrame(model.getGround());
    InitializeModel(model);
    InitializeState(model);
    ASSERT_NO_THROW({ ToOscMesh(model, model.getWorkingState(), mesh); });
}

// generate decorations should only generate decorations for the provided model's
// _subcomponents_, because the model itself will effectively double-generate
// everything and label it with 'model
TEST(OpenSimDecorationGenerator, DoesntIncludeTheModelsDirectDecorations)
{
    std::filesystem::path const tugOfWarPath = std::filesystem::path{OSC_TESTING_SOURCE_DIR} / "resources" / "models" / "Tug_of_War" / "Tug_of_War.osim";
    OpenSim::Model model{tugOfWarPath.string()};
    InitializeModel(model);
    InitializeState(model);
    SceneCache meshCache;
    OpenSimDecorationOptions opts;

    bool empty = true;
    GenerateModelDecorations(
        meshCache,
        model,
        model.getWorkingState(),
        opts,
        1.0f,
        [&model, &empty](OpenSim::Component const& c, SceneDecoration&&)
        {
            ASSERT_NE(&c, &model);
            empty = false;
        }
    );
    ASSERT_FALSE(empty);
}

// generate model decorations with collision arrows should work fine for the soccerkick model
//
// (this is just an automated repro for that one time where I screwed up a loop in the renderer ;))
TEST(OpenSimDecorationGenerator, GenerateCollisionArrowsWorks)
{
    std::filesystem::path const soccerKickPath = std::filesystem::path{OSC_TESTING_SOURCE_DIR} / "resources" / "models" / "SoccerKick" / "SoccerKickingModel.osim";
    OpenSim::Model model{soccerKickPath.string()};
    InitializeModel(model);
    InitializeState(model);
    SceneCache meshCache;

    OpenSimDecorationOptions opts;
    opts.setShouldShowContactForces(true);

    bool empty = true;
    GenerateModelDecorations(
        meshCache,
        model,
        model.getWorkingState(),
        opts,
        1.0f,
        [&empty](OpenSim::Component const&, SceneDecoration&&) { empty = false; }
    );
    ASSERT_FALSE(empty);
}

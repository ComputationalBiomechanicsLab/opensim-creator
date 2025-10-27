#include "OpenSimDecorationGenerator.h"

#include <libopensimcreator/Documents/Model/BasicModelStatePair.h>
#include <libopensimcreator/Graphics/ComponentAbsPathDecorationTagger.h>
#include <libopensimcreator/Graphics/MuscleColorSource.h>
#include <libopensimcreator/Graphics/OpenSimDecorationOptions.h>
#include <libopensimcreator/Platform/OpenSimCreatorApp.h>
#include <libopensimcreator/testing/TestOpenSimCreatorConfig.h>
#include <libopensimcreator/Utils/OpenSimHelpers.h>

#include <gtest/gtest.h>
#include <liboscar/Graphics/Scene/SceneCache.h>
#include <liboscar/Graphics/Scene/SceneDecoration.h>
#include <liboscar/Graphics/Scene/SceneHelpers.h>
#include <liboscar/Maths/AABBFunctions.h>
#include <liboscar/Maths/MathHelpers.h>
#include <liboscar/Platform/Log.h>
#include <liboscar/Utils/StringHelpers.h>
#include <OpenSim/Simulation/Model/ModelComponent.h>
#include <OpenSim/Simulation/Model/ContactGeometry.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/Ligament.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <algorithm>
#include <filesystem>
#include <limits>
#include <ranges>
#include <utility>
#include <variant>
#include <vector>

using namespace osc;
namespace rgs = std::ranges;

// test that telling OSC to generate OpenSim-colored muscles
// results in red muscle lines (as opposed to muscle lines that
// are based on something like excitation - #663)
TEST(OpenSimDecorationGenerator, GenerateDecorationsWithOpenSimMuscleColoringGeneratesRedMuscles)
{
    GloballyInitOpenSim();  // ensure component registry is populated

    // TODO: this should be more synthetic and should just create a body with one muscle with a
    // known color that is then pumped through the pipeline etc.
    const std::filesystem::path tugOfWarPath = std::filesystem::path{OSC_RESOURCES_DIR} / "OpenSimCreator/models/Tug_of_War/Tug_of_War.osim";
    OpenSim::Model model{tugOfWarPath.string()};
    model.buildSystem();
    SimTK::State& state = model.initializeState();

    OpenSimDecorationOptions opts;
    opts.setMuscleColorSource(MuscleColorSource::AppearanceProperty);

    SceneCache meshCache;
    bool passedTest = false;
    GenerateModelDecorations(
        meshCache,
        model,
        state,
        opts,
        1.0f,
        [&passedTest](const OpenSim::Component& c, const SceneDecoration& dec)
        {
            if (contains_case_insensitive(c.getName(), "muscle1")) {

                ASSERT_TRUE(std::holds_alternative<Color>(dec.shading)) << "should have an assigned color";

                // check that it's red
                const auto& color = std::get<Color>(dec.shading);
                ASSERT_GT(color.r, 0.5f);
                ASSERT_GT(color.r, 5.0f*color.g);
                ASSERT_GT(color.r, 5.0f*color.b);

                // and that it casts shadows (rando bug in 0.5.9)
                ASSERT_FALSE(dec.flags & SceneDecorationFlag::NoCastsShadows);
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
    const SimTK::State& state = model.initializeState();

    const auto generateDecorationsWithScaleFactor = [&model, &state](float scaleFactor)
    {
        SceneCache meshCache;

        std::vector<SceneDecoration> rv;
        GenerateModelDecorations(
            meshCache,
            model,
            state,
            OpenSimDecorationOptions{},
            scaleFactor,
            [&rv](const OpenSim::Component& c, SceneDecoration&& dec)
            {
                // only suck up the frame decorations associated with ground
                if (dynamic_cast<const OpenSim::Ground*>(&c))
                {
                    rv.push_back(std::move(dec));
                }
            }
        );
        return rv;
    };

    const float scale = 0.25f;
    const std::vector<SceneDecoration> unscaledDecs = generateDecorationsWithScaleFactor(1.0f);
    const std::vector<SceneDecoration> scaledDecs = generateDecorationsWithScaleFactor(scale);

    ASSERT_FALSE(unscaledDecs.empty());
    ASSERT_FALSE(scaledDecs.empty());
    ASSERT_EQ(unscaledDecs.size(), scaledDecs.size());

    for (size_t i = 0; i < unscaledDecs.size(); ++i)
    {
        const SceneDecoration& unscaledDec = unscaledDecs[i];
        const SceneDecoration& scaledDec = scaledDecs[i];

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
        const OpenSim::Geometry* geomPtr = geom.get();

        body->attachGeometry(geom.release());
        m.addBody(body.release());
        m.buildSystem();

        return std::pair{std::move(m), geomPtr->getAbsolutePath()};
    }();
    p.first.buildSystem();
    const SimTK::State& state = p.first.initializeState();

    // helper
    const auto generateDecorationsWithScaleFactor = [&p, &state](float scaleFactor)
    {
        SceneCache meshCache;

        std::vector<SceneDecoration> rv;
        GenerateModelDecorations(
            meshCache,
            p.first,
            state,
            OpenSimDecorationOptions{},
            scaleFactor,
            [&p, &rv](const OpenSim::Component& c, SceneDecoration&& dec)
            {
                if (c.getAbsolutePath() == p.second)
                {
                    rv.push_back(std::move(dec));
                }
            }
        );
        return rv;
    };

    const float scale = 0.25f;
    const std::vector<SceneDecoration> unscaledDecs = generateDecorationsWithScaleFactor(1.0f);
    const std::vector<SceneDecoration> scaledDecs = generateDecorationsWithScaleFactor(scale);

    ASSERT_FALSE(unscaledDecs.empty());
    ASSERT_FALSE(scaledDecs.empty());
    ASSERT_EQ(unscaledDecs.size(), scaledDecs.size());

    for (size_t i = 0; i < unscaledDecs.size(); ++i) {
        const SceneDecoration& unscaledDec = unscaledDecs[i];
        const SceneDecoration& scaledDec = scaledDecs[i];

        // note: not scaled
        ASSERT_TRUE(all_of(equal_within_reldiff(unscaledDec.transform.scale, scaledDec.transform.scale, 0.0001f)));
    }
}

TEST(OpenSimDecorationGenerator, ToOscMeshWorksAsIntended)
{
    const std::filesystem::path arrowPath = std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "arrow.vtp";

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
    GloballyInitOpenSim();  // ensure component registry is initialized

    const std::filesystem::path tugOfWarPath = std::filesystem::path{OSC_RESOURCES_DIR} / "OpenSimCreator/models/Tug_of_War/Tug_of_War.osim";
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
        [&model, &empty](const OpenSim::Component& c, const SceneDecoration&)
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
    GloballyInitOpenSim();  // ensure component registry is initialized

    const std::filesystem::path soccerKickPath = std::filesystem::path{OSC_RESOURCES_DIR} / "OpenSimCreator/models/SoccerKick/SoccerKickingModel.osim";
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
        [&empty](const OpenSim::Component&, const SceneDecoration&) { empty = false; }
    );
    ASSERT_FALSE(empty);
}

// tests that, when generating decorations for an `OpenSim::Ligament`, the decorations are
// coerced from being `GeometryPath` decorations to `OpenSim::Ligament` decorations for the
// non-point parts of the path (#919)
TEST(OpenSimDecorationGenerator, GenerateDecorationsForLigamentGeneratesLigamentTaggedGeometry)
{
    OpenSim::Model model;
    auto ligamentptr = std::make_unique<OpenSim::Ligament>();
    ligamentptr->setRestingLength(1.0);  // required in debug mode :(
    auto& ligament = AddModelComponent(model, std::move(ligamentptr));
    auto pp1 = std::make_unique<OpenSim::PathPoint>();
    pp1->setLocation({-1.0, 0.0, 0.0});
    pp1->setParentFrame(model.getGround());

    auto pp2 = std::make_unique<OpenSim::PathPoint>();
    pp2->setLocation({1.0, 0.0, 0.0});
    pp2->setParentFrame(model.getGround());

    ligament.updPath<OpenSim::GeometryPath>().updPathPointSet().adoptAndAppend(pp1.release());
    ligament.updPath<OpenSim::GeometryPath>().updPathPointSet().adoptAndAppend(pp2.release());

    FinalizeConnections(model);
    InitializeModel(model);
    InitializeState(model);

    SceneCache meshCache;
    OpenSimDecorationOptions opts;

    size_t numDecorationsTaggedWithLigament = 0;
    GenerateModelDecorations(
        meshCache,
        model,
        model.getWorkingState(),
        opts,
        1.0f,
        [&ligament, &numDecorationsTaggedWithLigament](const OpenSim::Component& component, const SceneDecoration&)
        {
            if (&component == &ligament) {
                ++numDecorationsTaggedWithLigament;
            }
        }
    );
    ASSERT_EQ(numDecorationsTaggedWithLigament, 1);
}

TEST(GenerateModelDecorations, ShortHandOverloadWithModelAndStateWorksAsExpected)
{
    GloballyInitOpenSim();  // ensure component registry is initialized

    // setup model + options
    const std::filesystem::path soccerKickPath = std::filesystem::path{OSC_RESOURCES_DIR} / "OpenSimCreator/models/SoccerKick/SoccerKickingModel.osim";
    OpenSim::Model model{soccerKickPath.string()};
    InitializeModel(model);
    InitializeState(model);
    SceneCache cache;
    OpenSimDecorationOptions opts;
    opts.setShouldShowContactForces(true);

    // emit decorations the hard way into a vector
    ComponentAbsPathDecorationTagger tagger;
    std::vector<SceneDecoration> decorations;
    GenerateModelDecorations(
        cache,
        model,
        model.getWorkingState(),
        opts,
        1.0f,
        [&tagger, &decorations](const OpenSim::Component& component, SceneDecoration&& decoration)
        {
            tagger(component, decoration);
            decorations.push_back(std::move(decoration));
        }
    );

    // now do it with the easy override
    const std::vector<SceneDecoration> easyDecorations = GenerateModelDecorations(cache, model, model.getWorkingState(), opts, 1.0);

    ASSERT_EQ(decorations, easyDecorations);
}

TEST(GenerateModelDecorations, ShortHandOverloadWithModelStatePairWorksAsExpected)
{
    GloballyInitOpenSim();  // ensure component registry is initialized

    // setup model + options
    const std::filesystem::path soccerKickPath = std::filesystem::path{OSC_RESOURCES_DIR} / "OpenSimCreator/models/SoccerKick/SoccerKickingModel.osim";
    const BasicModelStatePair modelState{soccerKickPath.string()};
    SceneCache cache;
    OpenSimDecorationOptions opts;
    opts.setShouldShowContactForces(true);

    // emit decorations the hard way into a vector
    ComponentAbsPathDecorationTagger tagger;
    std::vector<SceneDecoration> decorations;
    GenerateModelDecorations(
        cache,
        modelState.getModel(),
        modelState.getState(),
        opts,
        1.0f,
        [&tagger, &decorations](const OpenSim::Component& component, SceneDecoration&& decoration)
        {
            tagger(component, decoration);
            decorations.push_back(std::move(decoration));
        }
    );

    // now do it with the easy override
    const std::vector<SceneDecoration> easyDecorations = GenerateModelDecorations(cache, modelState, opts, 1.0);

    ASSERT_EQ(decorations, easyDecorations);
}

// user reported that `OpenSim::ContactGeometry` cannot be toggled _off_ via its
// `Appearance::is_visible` flag (#980).
//
// This test ensures the reverse (i.e. when it is visible) operates within expectations.
TEST(GenerateModelDecorations, GeneratesContactGeometrySphereWhenVisibilityFlagIsEnabled)
{
    OpenSim::Model model;
    auto* sphere = new OpenSim::ContactSphere(1.0, SimTK::Vec3{0.0}, model.getGround());
    model.addContactGeometry(sphere);
    model.buildSystem();
    const SimTK::State& state = model.initializeState();

    SceneCache cache;
    OpenSimDecorationOptions opts;
    const auto decorations = GenerateModelDecorations(cache, model, state);
    const auto isContactSphereDecoration = [p = sphere->getAbsolutePathString()](const SceneDecoration& dec) { return dec.id == p; };

    ASSERT_EQ(rgs::count_if(decorations, isContactSphereDecoration), 1);
}

// user reported that `OpenSim::ContactGeometry` cannot be toggled _off_ via its
// `Appearance::is_visible` flag (#980).
//
// This test checks that turning the flag off prevents the decoration generator from
// generating a decoration for it.
TEST(GenerateModelDecorations, DoesNotGenerateContactGeometrySphereWhenVisibilityFlagIsDisabled)
{
    OpenSim::Model model;
    auto* sphere = new OpenSim::ContactSphere(1.0, SimTK::Vec3{0.0}, model.getGround());
    sphere->upd_Appearance().set_visible(false);  // should prevent it from emitting
    model.addContactGeometry(sphere);
    model.buildSystem();
    const SimTK::State& state = model.initializeState();

    SceneCache cache;
    OpenSimDecorationOptions opts;
    const auto decorations = GenerateModelDecorations(cache, model, state);
    const auto isContactSphereDecoration = [p = sphere->getAbsolutePathString()](const SceneDecoration& dec) { return dec.id == p; };

    ASSERT_EQ(rgs::count_if(decorations, isContactSphereDecoration), 0);
}

namespace
{
    // A mock component that emits a cylinder with NaN radius.
    class ComponentThatGeneratesNaNCylinder : public OpenSim::ModelComponent {
        OpenSim_DECLARE_CONCRETE_OBJECT(ComponentThatGeneratesNaNCylinder, OpenSim::ModelComponent)

    public:
        void generateDecorations(
            bool fixed,
            const OpenSim::ModelDisplayHints&,
            const SimTK::State&,
            SimTK::Array_<SimTK::DecorativeGeometry>& out) const final
        {
            if (fixed) {
                return;
            }
            SimTK::DecorativeCylinder cylinder{std::numeric_limits<double>::quiet_NaN(), 0.5};
            out.push_back(cylinder);
        }
    };
}

// This was found when diagnosing an `OpenSim::ExpressionBasedBushingForce`. The OpenSim
// `generateDecorations` backend was generating NaNs for the object's transform, which was
// propagating through to the renderer and causing hittest issues (#976).
TEST(GenerateModelDecorations, FiltersOutCylinderWithNANRadius)
{
    OpenSim::Model model;
    model.updDisplayHints().set_show_frames(false);
    model.addModelComponent(std::make_unique<ComponentThatGeneratesNaNCylinder>().release());
    model.buildSystem();
    const SimTK::State& state = model.initializeState();

    SceneCache cache;
    OpenSimDecorationOptions opts;
    const auto decorations = GenerateModelDecorations(cache, model, state);

    ASSERT_EQ(decorations.size(), 0);
}

namespace
{
    // A mock component that generates `SimTK::DecorativeSphere`s with NaNed rotations.
    class ComponentThatGeneratesNaNRotationSphere : public OpenSim::ModelComponent {
        OpenSim_DECLARE_CONCRETE_OBJECT(ComponentThatGeneratesNaNRotationSphere, OpenSim::ModelComponent)

    public:
        void generateDecorations(
            bool fixed,
            const OpenSim::ModelDisplayHints&,
            const SimTK::State&,
            SimTK::Array_<SimTK::DecorativeGeometry>& out) const final
        {
            if (fixed) {
                return;
            }
            SimTK::DecorativeSphere sphere;
            sphere.setTransform(SimTK::Transform{
                // NaNed rotation
                SimTK::Rotation{std::numeric_limits<double>::quiet_NaN(), SimTK::CoordinateAxis::XCoordinateAxis{}},
                SimTK::Vec3{0.0},
            });
            out.push_back(sphere);
        }
    };
}

// This was found when simulating `arm26.osim`. A forward-dynamic simulation of it exploded
// for some probably-physics-related internal reason, and that resulted in the backend emitting
// geometry containing NaNed transforms. That geometry then propagated through the UI and caused
// mayhem related to hittesting and finding scene bounds (#976).
TEST(GenerateModelDecorations, FiltersOutSpheresWithNaNRotations)
{
    OpenSim::Model model;
    model.updDisplayHints().set_show_frames(false);
    model.addModelComponent(std::make_unique<ComponentThatGeneratesNaNRotationSphere>().release());
    model.buildSystem();
    const SimTK::State& state = model.initializeState();

    SceneCache cache;
    OpenSimDecorationOptions opts;
    const auto decorations = GenerateModelDecorations(cache, model, state);

    ASSERT_EQ(decorations.size(), 0);
}

namespace
{
    // A mock component that generates `SimTK::DecorativeSphere`s with NaNed translation.
    class ComponentThatGeneratesNaNTranslationSphere : public OpenSim::ModelComponent {
        OpenSim_DECLARE_CONCRETE_OBJECT(ComponentThatGeneratesNaNTranslationSphere, OpenSim::ModelComponent)

    public:
        void generateDecorations(
            bool fixed,
            const OpenSim::ModelDisplayHints&,
            const SimTK::State&,
            SimTK::Array_<SimTK::DecorativeGeometry>& out) const final
        {
            if (fixed) {
                return;
            }
            SimTK::DecorativeSphere sphere;
            sphere.setTransform(SimTK::Transform{
                // NaNed rotation
                SimTK::Rotation{},
                SimTK::Vec3{std::numeric_limits<double>::quiet_NaN()},
            });
            out.push_back(sphere);
        }
    };
}

// This was found when simulating `arm26.osim`. A forward-dynamic simulation of it exploded
// for some probably-physics-related internal reason, and that resulted in the backend emitting
// geometry containing NaNed transforms. That geometry then propagated through the UI and caused
// mayhem related to hittesting and finding scene bounds (#976).
TEST(GenerateModelDecorations, FiltersOutSpheresWithNaNTranslation)
{
    OpenSim::Model model;
    model.updDisplayHints().set_show_frames(false);
    model.addModelComponent(std::make_unique<ComponentThatGeneratesNaNTranslationSphere>().release());
    model.buildSystem();
    const SimTK::State& state = model.initializeState();

    SceneCache cache;
    OpenSimDecorationOptions opts;
    const auto decorations = GenerateModelDecorations(cache, model, state);

    ASSERT_EQ(decorations.size(), 0);
}

// Test for a regression found during `Scholz2015GeometryPath` integration (#1131)
//
// Upstream `opensim-core`, around v4.6, added a `SimTK::ContactGeometry` cache to
// `OpenSim::ContactGeometry`, which created invalid behavior in which changing an
// `OpenSim::ContactGeometry` derived property (e.g. `radius` on `ContactSphere`)
// wouldn't update the associated decoration because an intermediate cached pointer
// wasn't being invalidated.
TEST(GenerateModelDecorations, RadiusOfContactSphereIsCorrectlyUpdated)
{
    OpenSim::Model model;
    auto& sphere = osc::AddComponent<OpenSim::ContactSphere>(model);
    sphere.setRadius(0.1);
    sphere.setFrame(model.getGround());
    model.buildSystem();
    const SimTK::State& state = model.initializeState();

    SceneCache cache;
    OpenSimDecorationOptions opts;

    // Before changing radius: it should be as-set
    {
        const auto decorations = GenerateModelDecorations(cache, model, state);
        const float volume = volume_of(bounding_aabb_of(decorations, world_space_bounds_of));
        ASSERT_NEAR(volume, 0.2f*0.2f*0.2f, 0.001f);
    }

    sphere.setRadius(0.5);
    model.buildSystem();
    model.initializeState();

    // After changing radius: should update it
    {
        const auto decorations = GenerateModelDecorations(cache, model, state);
        const float volume = volume_of(bounding_aabb_of(decorations, world_space_bounds_of));
        ASSERT_NEAR(volume, 1.0f*1.0f*1.0f, 0.001f);
    }
}

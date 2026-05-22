#include "simbody_decoration_generator.h"

#include <libopynsim/utilities/simbody_x_oscar.h>
#include <liboscar/graphics/scene/scene_cache.h>
#include <liboscar/graphics/scene/scene_decoration.h>
#include <liboscar/graphics/scene/scene_decoration_flags.h>

#include <gtest/gtest.h>
#include <simbody/internal/MultibodySystem.h>
#include <simbody/internal/SimbodyMatterSubsystem.h>

using namespace opyn;

// ensure the SimTKDecorationGenerator correctly tags emitted geometry with
// a wireframe flag when given a wireframe representation decoration
TEST(SimTKDecorationGenerator, PropagatesWireframeShadingFlag)
{
    osc::SceneCache cache;

    SimTK::MultibodySystem sys;
    SimTK::SimbodyMatterSubsystem matter{sys};
    SimTK::State state = sys.realizeTopology();
    sys.realize(state);

    SimTK::DecorativeSphere sphere;
    sphere.setBodyId(0);
    sphere.setRepresentation(SimTK::DecorativeGeometry::DrawWireframe);

    size_t ncalls = 0;
    GenerateDecorations(cache, matter, state, sphere, 1.0f, [&ncalls](const osc::SceneDecoration& dec)
    {
        ++ncalls;
        ASSERT_TRUE(dec.flags & osc::SceneDecorationFlag::DrawWireframeOverlay);
    });
    ASSERT_EQ(ncalls, 1) << "should only emit one is_wireframe sphere";
}

// ensure the SimTKDecorationGenerator correctly tags emitted geometry as
// not drawn when given a hidden representation
TEST(SimTKDecorationGenerator, PropagatesHiddenRepresentation)
{
    osc::SceneCache cache;

    SimTK::MultibodySystem sys;
    SimTK::SimbodyMatterSubsystem matter{sys};
    SimTK::State state = sys.realizeTopology();
    sys.realize(state);

    SimTK::DecorativeSphere sphere;
    sphere.setBodyId(0);
    sphere.setRepresentation(SimTK::DecorativeGeometry::Hide);

    size_t ncalls = 0;
    GenerateDecorations(cache, matter, state, sphere, 1.0f, [&ncalls](const osc::SceneDecoration& dec)
    {
        ++ncalls;
        ASSERT_TRUE(dec.flags & osc::SceneDecorationFlag::NoDrawInScene);
    });
    ASSERT_EQ(ncalls, 1) << "should only emit one is_wireframe sphere";
}

// ensure that the `SimTKDecorationGenerator` propagates negative scale factors,
// because some users use them to mirror-image geometry (#974)
TEST(SimTKDecorationGenerator, PropagatesNegativeScaleFactors)
{
    osc::SceneCache cache;

    SimTK::MultibodySystem sys;
    SimTK::SimbodyMatterSubsystem matter{sys};
    SimTK::State state = sys.realizeTopology();
    sys.realize(state);

    SimTK::DecorativeSphere sphere;
    sphere.setBodyId(0);
    sphere.setRepresentation(SimTK::DecorativeGeometry::Hide);
    sphere.setRadius(1.0);
    sphere.setScaleFactors(SimTK::Vec3(1.0, -1.0, 1.0));  // note: negative

    GenerateDecorations(cache, matter, state, sphere, 1.0f, [&](const osc::SceneDecoration& dec)
    {
        ASSERT_EQ(dec.transform.scale.y(), -1.0f);
    });
}

TEST(SimTKDecorationGenerator, UsesColorOverrideWhenEmittingFrames)
{
    osc::SceneCache cache;

    SimTK::MultibodySystem sys;
    SimTK::SimbodyMatterSubsystem matter{sys};
    SimTK::State state = sys.realizeTopology();
    sys.realize(state);

    const osc::Color overrideColor = osc::Color::red();

    SimTK::DecorativeFrame frame;
    frame.setBodyId(0);
    frame.setColor(to<SimTK::Vec3>(overrideColor));

    GenerateDecorations(cache, matter, state, frame, 1.0f, [&](const osc::SceneDecoration& dec)
    {
        ASSERT_TRUE(std::holds_alternative<osc::Color>(dec.shading));
        ASSERT_EQ(std::get<osc::Color>(dec.shading), overrideColor);
    });
}

// The `SimTK::DecorativeGeometry` API will use a scale factor of `-1` to indicate
// a defaulted axis. This scale factor shouldn't be propagated (i.e. it should
// be defaulted) to OPynSim; otherwise, visual glitches will occur
// (ComputationalBiomechanicsLab/opensim-creator#1179).
//
// However, if a caller explicitly sets `{-1, -1, -1}` (or any negative axis) on geometry
// then it should be handled as "Flip that axis" (i.e. a cheap-and-nasty way to mirror a mesh,
// ComputationalBiomechanicsLab/opensim-creator#974).
TEST(SimTKDecorationGenerator, Emits111WhenGivenGeometryWithDefaultedScaleFactors)
{
    osc::SceneCache cache;

    SimTK::MultibodySystem sys;
    SimTK::SimbodyMatterSubsystem matter{sys};
    SimTK::State state = sys.realizeTopology();
    sys.realize(state);

    // Defaulting it should just propagate the brick edge-lengths (ComputationalBiomechanicsLab/opensim-creator#1179).
    {
        SimTK::DecorativeBrick brick{SimTK::Vec3{0.02, 0.01, 0.005}};
        brick.setColor(SimTK::Orange);
        GenerateDecorations(cache, matter, state, brick, 1.0f, [&](const osc::SceneDecoration& dec)
        {
            ASSERT_EQ(dec.transform.scale, osc::Vector3f(0.02f, 0.01f, 0.005f));
        });
    }

    // Explicitly setting an axis to -1 should propagate it, though (ComputationalBiomechanicsLab/opensim-creator#974).
    {
        SimTK::DecorativeBrick scaledBrick{SimTK::Vec3{0.02, 0.01, 0.005}};
        scaledBrick.setColor(SimTK::Orange);
        scaledBrick.setScaleFactors({-1.0, 1.0, 1.0});
        GenerateDecorations(cache, matter, state, scaledBrick, 1.0f, [&](const osc::SceneDecoration& dec)
        {
            ASSERT_EQ(dec.transform.scale, osc::Vector3f(-0.02f, 0.01f, 0.005f));
        });
    }
}

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

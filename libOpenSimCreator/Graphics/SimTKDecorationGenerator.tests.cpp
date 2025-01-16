#include "SimTKDecorationGenerator.h"

#include <gtest/gtest.h>
#include <liboscar/oscar.h>
#include <Simbody.h>

using namespace osc;

// ensure the SimTKDecorationGenerator correctly tags emitted geometry with
// a wireframe flag when given a wireframe representation decoration
TEST(SimTKDecorationGenerator, PropagatesWireframeShadingFlag)
{
    SceneCache cache;

    SimTK::MultibodySystem sys;
    SimTK::SimbodyMatterSubsystem matter{sys};
    SimTK::State state = sys.realizeTopology();
    sys.realize(state);

    SimTK::DecorativeSphere sphere;
    sphere.setBodyId(0);
    sphere.setRepresentation(SimTK::DecorativeGeometry::DrawWireframe);

    size_t ncalls = 0;
    GenerateDecorations(cache, matter, state, sphere, 1.0f, [&ncalls](const SceneDecoration& dec)
    {
        ++ncalls;
        ASSERT_TRUE(dec.flags & SceneDecorationFlag::DrawWireframeOverlay);
    });
    ASSERT_EQ(ncalls, 1) << "should only emit one is_wireframe sphere";
}

// ensure the SimTKDecorationGenerator correctly tags emitted geometry as
// not drawn when given a hidden representation
TEST(SimTKDecorationGenerator, PropagatesHiddenRepresentation)
{
    SceneCache cache;

    SimTK::MultibodySystem sys;
    SimTK::SimbodyMatterSubsystem matter{sys};
    SimTK::State state = sys.realizeTopology();
    sys.realize(state);

    SimTK::DecorativeSphere sphere;
    sphere.setBodyId(0);
    sphere.setRepresentation(SimTK::DecorativeGeometry::Hide);

    size_t ncalls = 0;
    osc::GenerateDecorations(cache, matter, state, sphere, 1.0f, [&ncalls](const SceneDecoration& dec)
    {
        ++ncalls;
        ASSERT_TRUE(dec.flags & SceneDecorationFlag::NoDrawInScene);
    });
    ASSERT_EQ(ncalls, 1) << "should only emit one is_wireframe sphere";
}

// ensure that the `SimTKDecorationGenerator` propagates negative scale factors,
// because some users use them to mirror-image geometry (#974)
TEST(SimTKDecorationGenerator, PropagatesNegativeScaleFactors)
{
    SceneCache cache;

    SimTK::MultibodySystem sys;
    SimTK::SimbodyMatterSubsystem matter{sys};
    SimTK::State state = sys.realizeTopology();
    sys.realize(state);

    SimTK::DecorativeSphere sphere;
    sphere.setBodyId(0);
    sphere.setRepresentation(SimTK::DecorativeGeometry::Hide);
    sphere.setRadius(1.0);
    sphere.setScaleFactors(SimTK::Vec3(1.0, -1.0, 1.0));  // note: negative

    osc::GenerateDecorations(cache, matter, state, sphere, 1.0f, [&](const SceneDecoration& dec)
    {
        ASSERT_EQ(dec.transform.scale.y, -1.0f);
    });
}

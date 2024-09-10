#include <oscar_simbody/SimTKDecorationGenerator.h>

#include <gtest/gtest.h>
#include <oscar/oscar.h>
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
    GenerateDecorations(cache, matter, state, sphere, 1.0f, [&ncalls](SceneDecoration&& dec)
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
    osc::GenerateDecorations(cache, matter, state, sphere, 1.0f, [&ncalls](SceneDecoration&& dec)
    {
        ++ncalls;
        ASSERT_TRUE(dec.flags & SceneDecorationFlag::NoDrawInScene);
    });
    ASSERT_EQ(ncalls, 1) << "should only emit one is_wireframe sphere";
}

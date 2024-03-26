#include <OpenSimCreator/Documents/CustomComponents/InMemoryMesh.h>

#include <gtest/gtest.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>
#include <oscar/Graphics/Scene/SceneDecoration.h>

using namespace osc;
using namespace osc::mow;

TEST(InMemoryMesh, CanDefaultConstruct)
{
    ASSERT_NO_THROW({ InMemoryMesh instance; });
}

TEST(InMemoryMesh, DefaultConstructedEmitsABlankMesh)
{
    OpenSim::Model model;
    auto& mesh = AddComponent<InMemoryMesh>(model);
    mesh.connectSocket_frame(model.getGround());
    FinalizeConnections(model);
    InitializeModel(model);
    SimTK::State& state = InitializeState(model);

    int nEmitted = 0;
    SceneDecoration latestDecoration;
    mesh.generateCustomDecorations(state, [&nEmitted, &latestDecoration](SceneDecoration&& decoration)
    {
        ++nEmitted;
        latestDecoration = std::move(decoration);
    });

    ASSERT_EQ(nEmitted, 1);
    ASSERT_EQ(latestDecoration.mesh.getNumVerts(), 0);
    ASSERT_EQ(latestDecoration.mesh.getNumIndices(), 0);
}

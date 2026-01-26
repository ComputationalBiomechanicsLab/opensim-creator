#include "in_memory_mesh.h"

#include <gtest/gtest.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/graphics/scene/scene_decoration.h>
#include <OpenSim/Simulation/Model/Model.h>

using namespace opyn;
using osc::mow::InMemoryMesh;

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
    osc::SceneDecoration latestDecoration;
    mesh.generateCustomDecorations(state, [&nEmitted, &latestDecoration](osc::SceneDecoration&& decoration)
    {
        ++nEmitted;
        latestDecoration = std::move(decoration);
    });

    ASSERT_EQ(nEmitted, 1);
    ASSERT_EQ(latestDecoration.mesh.num_vertices(), 0);
    ASSERT_EQ(latestDecoration.mesh.num_indices(), 0);
}

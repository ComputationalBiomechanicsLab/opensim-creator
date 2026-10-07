#include "in_memory_mesh.h"

#include <gtest/gtest.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/graphics/scene/scene_decoration.h>
#include <OpenSim/Simulation/Model/Model.h>

using namespace opyn;

TEST(InMemoryMesh, is_default_constructible)
{
    ASSERT_NO_THROW({ InMemoryMesh instance; });
}

TEST(InMemoryMesh, default_constructed_instance_emits_blank_mesh)
{
    OpenSim::Model model;
    auto& mesh = AddComponent<InMemoryMesh>(model);
    mesh.connectSocket_frame(model.getGround());
    FinalizeConnections(model);
    InitializeModel(model);
    SimTK::State& state = InitializeState(model);

    int num_decorations_emitted = 0;
    osc::SceneDecoration last_decoration;
    mesh.generate_custom_decorations(state, [&num_decorations_emitted, &last_decoration](osc::SceneDecoration&& decoration)
    {
        ++num_decorations_emitted;
        last_decoration = std::move(decoration);
    });

    ASSERT_EQ(num_decorations_emitted, 1);
    ASSERT_EQ(last_decoration.mesh.num_vertices(), 0);
    ASSERT_EQ(last_decoration.mesh.num_indices(), 0);
}

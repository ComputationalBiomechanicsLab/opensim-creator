#include "model_specification.h"

#include <libopynsim/documents/custom_components/in_memory_mesh.h>
#include <libopynsim/tests/opynsim_tests_config.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <libopynsim/opynsim.h>

#include <gtest/gtest.h>
#include <liboscar/graphics/geometries/box_geometry.h>
#include <liboscar/graphics/geometries/sphere_geometry.h>
#include <liboscar/utilities/temporary_directory.h>
#include <liboscar/utilities/temporary_file.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/StationDefinedFrame.h>
#include <OpenSim/Simulation/SimbodyEngine/Body.h>
#include <OpenSim/Simulation/SimbodyEngine/PinJoint.h>

#include <numbers>
#include <utility>

using namespace opyn;

TEST(ModelSpecification, compile_works_on_blank_ModelSpecification)
{
    opyn::init();

    const ModelSpecification model_specification;
    ASSERT_NO_THROW({ model_specification.compile(); });
}

TEST(ModelSpecification, compile_works_on_more_complicated_example_OpenSim_model)
{
    opyn::init();

    const ModelSpecification model_specification = read_osim(opynsim_tests_resources_directory() / "models/RajagopalModel/Rajagopal2015.osim");
    ASSERT_NO_THROW({ model_specification.compile(); });
}

TEST(ModelSpecification, can_construct_from_opensim_model)
{
    // The `OpenSim::Model` constructor is mostly there for legacy compatibility
    // with the various solvers etc. that emit `OpenSim::Model`s, but it should
    // work for now!
    opyn::init();

    OpenSim::Model opensim_model;
    auto& head = AddBody(
        opensim_model,
        "head",
        1.0,
        SimTK::Vec3{0.0},
        SimTK::Inertia{SimTK::Vec3{1.0}}
    );
    auto& pin = AddJoint<OpenSim::PinJoint>(
        opensim_model,
        "pin",
        opensim_model.getGround(),
        SimTK::Vec3{0.0, 0.0, 0.0},
        SimTK::Vec3{0.0},
        head,
        SimTK::Vec3{0.0, -1.0, 0.0},
        SimTK::Vec3{0.0}
    );
    pin.updCoordinate().set_default_value(0.25*std::numbers::pi_v<double>);
    opensim_model.finalizeConnections();

    const ModelSpecification model_specification{std::move(opensim_model)};
    const Model model = model_specification.compile();

    ASSERT_EQ(model.num_coordinates(), 1);
    ASSERT_EQ(model.coordinates(), std::vector{Symbol{"/jointset/pin/pin_coord_0"}});
}

TEST(ModelSpecification, root_directory_is_curdir_for_default_initialized_instance)
{
    ASSERT_EQ(ModelSpecification{}.root_directory(), std::filesystem::path{"."});
}

TEST(ModelSpecification, root_directory_is_osim_parent_when_loaded_from_osim)
{
    opyn::init();

    const std::filesystem::path p{opynsim_tests_resources_directory() / "models/Blank/blank.osim"};
    const ModelSpecification model_specification = read_osim(p);
    ASSERT_EQ(model_specification.root_directory(), p.parent_path());
}

TEST(ModelSpecification, root_directory_is_osim_parent_when_loaded_via_opensim_model)
{
    opyn::init();

    const std::filesystem::path p{opynsim_tests_resources_directory() / "models/Blank/blank.osim"};
    OpenSim::Model opensim_model{p.string()};
    const ModelSpecification model_specification{std::move(opensim_model)};

    ASSERT_EQ(model_specification.root_directory(), p.parent_path());
}

TEST(ModelSpecification, set_root_directory_makes_root_directory_return_same_string)
{
    opyn::init();

    ModelSpecification model_specification;
    ASSERT_NE(model_specification.root_directory(), "some/relative/path");
    model_specification.set_root_directory("some/relative/path");
    ASSERT_EQ(model_specification.root_directory(), "some/relative/path");
}

TEST(ModelSpecification, set_root_directory_on_model_loaded_from_osim_also_works)
{
    // This is just a sanity check that ensures `ModelSpecifications` that are
    // loaded from files behave identically to those created in-memory.

    opyn::init();

    const std::filesystem::path p{opynsim_tests_resources_directory() / "models/Blank/blank.osim"};
    ModelSpecification model_specification = read_osim(p);

    ASSERT_NE(model_specification.root_directory(), "some/relative/path");
    model_specification.set_root_directory("some/relative/path");
    ASSERT_EQ(model_specification.root_directory(), "some/relative/path");
}

TEST(ModelSpecification, to_osim_returns_a_string_that_opensim_can_read)
{
    opyn::init();

    // Create an example model specification.
    const auto model_specification = ModelSpecification::example_pendulum();

    // Write it to a temporary file.
    osc::TemporaryFile tmp{{.suffix = ".osim"}};
    tmp.close();
    model_specification.to_osim(tmp.absolute_path());

    // OpenSim should be able to read/finalize the file with no problems.
    OpenSim::Model model{tmp.absolute_path().string()};
    model.buildSystem();
}

TEST(ModelSpecification, to_osim_with_no_args_generates_string_that_is_identical_to_file_content)
{
    opyn::init();

    const auto model_specification = ModelSpecification::example_pendulum();

    osc::TemporaryFile tmp{{.suffix = ".osim"}};
    tmp.close();
    model_specification.to_osim(tmp.absolute_path());
    const std::string written_content{
        std::istreambuf_iterator<char>{std::ifstream{tmp.absolute_path()}.rdbuf()},
        std::istreambuf_iterator<char>{}
    };
    const std::string string_content = model_specification.to_osim();

    ASSERT_EQ(written_content, string_content);
}

TEST(ModelSpecification, bake_station_defined_frames_performs_conversion)
{
    // The reason why callers tend to use `bake_station_defined_frames`
    // is because they're about to write an osim that must be compatible with older versions
    // of OpenSim, so ensure that closed loop works.

    opyn::init();

    OpenSim::Model model;
    auto& origin = AddComponent<OpenSim::Station>(model, model.getGround(), SimTK::Vec3{0.0});
    auto& left   = AddComponent<OpenSim::Station>(model, model.getGround(), SimTK::Vec3{1.0, 0.0, 0.0});
    auto& up     = AddComponent<OpenSim::Station>(model, model.getGround(), SimTK::Vec3{0.0, 1.0, 0.0});
    AddComponent<OpenSim::StationDefinedFrame>(
        model,
        "sdf",
        SimTK::CoordinateAxis::XCoordinateAxis{},
        SimTK::CoordinateAxis::ZCoordinateAxis{},
        origin,
        left,
        up,
        origin
    );
    model.finalizeConnections();
    model.buildSystem();

    ModelSpecification model_specification{std::move(model)};
    ASSERT_TRUE(model_specification.to_osim().contains("<StationDefinedFrame"));
    ASSERT_FALSE(model_specification.to_osim().contains("<PhysicalOffsetFrame"));
    model_specification.bake_station_defined_frames();
    ASSERT_FALSE(model_specification.to_osim().contains("<StationDefinedFrame"));
    ASSERT_TRUE(model_specification.to_osim().contains("<PhysicalOffsetFrame"));
}

TEST(ModelSpecification, to_osim_throws_if_specification_contains_in_memory_mesh)
{
    // This tests very specific, but important, behavior.
    //
    // Some solvers in OPynSim/OpenSim Creator can temporarily add `InMemoryMesh`es
    // to `OpenSim::Model`s, so that the solver can produce+show a working result
    // quickly. However, an `InMemoryMesh` isn't serializable or writable to disk,
    // so `to_osim` should validate that the `ModelSpecification` does not contain
    // one and otherwise produce a helpful message that tells the caller how to
    // flush the meshes to disk.

    opyn::init();

    // Create a model containing an `InMemoryMesh`.
    const ModelSpecification model_specification = []
    {
        OpenSim::Model model;
        auto& in_memory_mesh = AddComponent<InMemoryMesh>(model);
        in_memory_mesh.setName("some_in_memory_mesh");
        in_memory_mesh.setFrame(model.getGround());
        model.finalizeConnections();
        return ModelSpecification{std::move(model)};
    }();

    // It should throw an exception if the caller tries to write it to a `std::string` with `to_osim`.
    try {
        model_specification.to_osim();
        FAIL() << "`to_osim` did not throw when the specification contains inlined resources (it should)";
    } catch (const std::runtime_error& ex) {
        ASSERT_TRUE(std::string_view{ex.what()}.contains("some_in_memory_mesh"));
        ASSERT_TRUE(std::string_view{ex.what()}.contains("flush_in_memory_resources_to"));
    }

    // It should also throw an exception if the caller tries to write it to a file with `to_osim`.
    osc::TemporaryFile tmp_file;
    tmp_file.close();
    try {
        model_specification.to_osim(tmp_file.absolute_path());
        FAIL() << "`to_osim` did not throw when the specification contains inlined resources (it should)";
    } catch (const std::runtime_error& ex) {
        ASSERT_TRUE(std::string_view{ex.what()}.contains("some_in_memory_mesh"));
        ASSERT_TRUE(std::string_view{ex.what()}.contains("flush_in_memory_resources_to"));
    }
}

TEST(ModelSpecification, flush_in_memory_resources_to_flushes_in_memory_meshes_to_directory)
{
    // This test ensures that creating an `OpenSim::Model` that contains
    // `InMemoryMesh`es can later be flushed to disk and replaced with
    // OpenSim-compatible `Mesh` geometry.

    opyn::init();

    // Create a model with some `InMemoryMesh`es.
    const osc::Mesh first_mesh = osc::BoxGeometry{};
    const osc::Mesh second_mesh = osc::SphereGeometry{};
    ModelSpecification model_specification = [&]
    {
        OpenSim::Model model;

        auto& body1 = AddBody(model, "body1", 1.0, SimTK::Vec3{0.0}, SimTK::Inertia{SimTK::Vec3{1.0}});
        auto& imm1 = AttachGeometry<InMemoryMesh>(body1, second_mesh);
        imm1.setName("in_memory_mesh1");
        imm1.setFrame(model.getGround());

        auto& body2 = AddBody(model, "body2", 1.0, SimTK::Vec3{0.0}, SimTK::Inertia{SimTK::Vec3{1.0}});
        auto& imm2 = AttachGeometry<InMemoryMesh>(body2, second_mesh);
        imm2.setName("in_memory_mesh2");
        imm2.setFrame(body2);
        model.finalizeConnections();
        return ModelSpecification{std::move(model)};
    }();

    // The specification shouldn't be serializable because it contains
    // `InMemoryMesh`es.
    ASSERT_ANY_THROW({ model_specification.to_osim(); });

    // Calling `ModelSpecification::flush_in_memory_resources_to` fixes that.
    osc::TemporaryDirectory tmp_dir;
    model_specification.flush_in_memory_resources_to(tmp_dir.absolute_path() / "flushed_resources");  // Should create directory, if necessary
    ASSERT_TRUE(model_specification.to_osim().contains("<Mesh"));

    // And the mesh files are written to disk with the expected names
    ASSERT_TRUE(std::filesystem::is_regular_file(tmp_dir.absolute_path() / "flushed_resources" / "in_memory_mesh1.obj"));
    ASSERT_TRUE(std::filesystem::is_regular_file(tmp_dir.absolute_path() / "flushed_resources" / "in_memory_mesh2.obj"));
}

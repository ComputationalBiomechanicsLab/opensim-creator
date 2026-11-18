#include "undoable_model_state_pair.h"

#include <libopensimcreator/platform/open_sim_creator_app.h>
#include <libopensimcreator/tests/testopensimcreatorconfig.h>

#include <gtest/gtest.h>
#include <libopynsim/graphics/open_sim_decoration_generator.h>
#include <libopynsim/graphics/open_sim_decoration_options.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/formats/dae.h>
#include <liboscar/graphics/scene/scene_cache.h>
#include <liboscar/graphics/scene/scene_decoration.h>
#include <liboscar/utilities/null_ostream.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/ExternalLoads.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <filesystem>
#include <functional>
#include <sstream>

using namespace osc;

TEST(UndoableModelStatePair, CanLoadAndRenderAllUserFacingExampleFiles)
{
    // ensure the OpenSim API is initialized and the meshes are loadable from
    // the central `geometry/` directory
    {
        globally_init_open_sim();
        globally_add_directory_to_open_sim_geometry_search_path(
            std::filesystem::path{OSC_RESOURCES_DIR} / "OpenSimCreator/geometry");
    }

    SceneCache mesh_cache;

    // turn as many decoration options on as possible, so that the code gets tested
    // against them (#661)
    opyn::OpenSimDecorationOptions decoration_opts;
    decoration_opts.set_should_show_everything(true);

    const std::filesystem::path examples_dir =
        std::filesystem::path{OSC_RESOURCES_DIR} / "OpenSimCreator/models";
    ASSERT_TRUE(
        std::filesystem::exists(examples_dir) && std::filesystem::is_directory(examples_dir));

    size_t n_examples_tested = 0;
    for (const std::filesystem::directory_entry& e :
         std::filesystem::recursive_directory_iterator{examples_dir}) {
        if (e.is_regular_file() and opyn::has_model_file_extension(e.path())) {
            // all files are loadable
            UndoableModelStatePair p{e.path()};

            // and all can be used to generate 3D scenes
            std::vector<SceneDecoration> decorations;
            generate_model_decorations(
                mesh_cache,
                p.get_model(),
                p.get_state(),
                decoration_opts,
                1.0f,  // 1:1 scaling
                [&decorations](const OpenSim::Component& component, SceneDecoration&& dec) {
                    dec.id = opyn::get_absolute_path_string_name(component);
                    decorations.push_back(std::move(dec));
                });

            // and decorations are generated
            ASSERT_FALSE(decorations.empty());

            ++n_examples_tested;
        }
    }
    ASSERT_GT(n_examples_tested, 10);  // sanity check: remove this if you want <10 examples
}

// this test just ensures that the DAE writer works for a reasonably complicated model
TEST(UndoableModelStatePair, canWriteRajagopalModelToDAE)
{
    // ensure the OpenSim API is initialized and the meshes are loadable from
    // the central `geometry/` directory
    {
        globally_init_open_sim();
        globally_add_directory_to_open_sim_geometry_search_path(
            std::filesystem::path{OSC_RESOURCES_DIR} / "OpenSimCreator/geometry");
    }

    // load model
    const UndoableModelStatePair p{
        std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "models" / "RajagopalModel" /
        "Rajagopal2015.osim"};

    // setup rendering state
    SceneCache mesh_cache;
    opyn::OpenSimDecorationOptions decoration_opts;
    decoration_opts.set_should_show_everything(true);

    // generate decorations
    std::vector<SceneDecoration> decorations;
    generate_model_decorations(
        mesh_cache,
        p.get_model(),
        p.get_state(),
        decoration_opts,
        1.0f,  // 1:1 scaling
        [&decorations](const OpenSim::Component& component, SceneDecoration&& dec) {
            dec.id = opyn::get_absolute_path_string_name(component);
            decorations.push_back(std::move(dec));
        });

    ASSERT_FALSE(decorations.empty()) << "decorations should be generated";

    // write decorations to a fake (testing) `std::ostream`
    NullOstream stream;
    const DAEMetadata metadata{
        TESTOPENSIMCREATOR_APPNAME_STRING, TESTOPENSIMCREATOR_APPNAME_STRING};
    DAE::write(stream, decorations, metadata);

    ASSERT_TRUE(stream.was_written_to()) << "the DAE writer should write content to the stream";
}

// related issue: #890
//
// calling `set_model` with an `OpenSim::Model` should retain the scene scale factor of
// the current scratch space
TEST(UndoableModelStatePair, setModelRetainsSceneScaleFactor)
{
    UndoableModelStatePair model;

    ASSERT_EQ(model.get_fixup_scale_factor(), 1.0f);
    model.set_fixup_scale_factor(0.5f);
    ASSERT_EQ(model.get_fixup_scale_factor(), 0.5f);

    model.set_model(std::make_unique<OpenSim::Model>());
    ASSERT_EQ(model.get_fixup_scale_factor(), 0.5f);
}

TEST(UndoableModelStatePair, resetModelRetainsSceneScaleFactor)
{
    UndoableModelStatePair model;

    ASSERT_EQ(model.get_fixup_scale_factor(), 1.0f);
    model.set_fixup_scale_factor(0.5f);
    ASSERT_EQ(model.get_fixup_scale_factor(), 0.5f);

    model.reset_model();
    ASSERT_EQ(model.get_fixup_scale_factor(), 0.5f);
}

// This is a repro for #924
//
// Grep #924 for a more comprehensive explanation, which is next to a lower-level test
TEST(UndoableModelStatePair, CanCommitWhenModelContainsExternalLoads)
{
    const std::filesystem::path example_model =
        std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "opensim-creator_924_repro.osim";
    const std::filesystem::path example_external_loads_file = std::filesystem::weakly_canonical(
        std::filesystem::path{OSC_TESTING_RESOURCES_DIR} /
        "opensim-creator_924_external-loads.xml");

    UndoableModelStatePair p{example_model};
    p.upd_model().addModelComponent(&dynamic_cast<OpenSim::ExternalLoads&>(
        *OpenSim::Object::makeObjectFromFile(example_external_loads_file.string())));
    ASSERT_NO_THROW({
        p.commit("this shouldn't throw if `OpenSim::ExternalLoads` is behaving itself");
    }) << "this shouldn't throw (see: opensim-core/3926 or opensim-core/3927)";
}

// repro for #1070
//
// User reported that they would like OSC to be able to edit models that have
// not-yet-optimized muscle parameters, so the system should ensure that it can
// load and initialize those kinds of models.
TEST(UndoableModelStatePair, CanLoadModelWithMuscleEquilibrationProblems)
{
    globally_init_open_sim();  // for loading the osim

    const std::filesystem::path broken_file_path =
        std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "opensim-creator_1070_repro.osim";
    ASSERT_NO_THROW({ UndoableModelStatePair{broken_file_path}; });
}

// no associated header: this is a top-level integration test

#include <TestOpenSimCreator/TestOpenSimCreatorConfig.h>

#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/Graphics/OpenSimDecorationOptions.h>
#include <OpenSimCreator/Graphics/OpenSimDecorationGenerator.h>
#include <OpenSimCreator/Platform/OpenSimCreatorApp.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/Scene/SceneCache.h>
#include <oscar/Graphics/Scene/SceneDecoration.h>
#include <oscar/Utils/FilesystemHelpers.h>

#include <array>
#include <filesystem>
#include <string_view>
#include <vector>

using namespace osc;

// sanity check: test that all user-facing `.osim` files in the documentation
// can be loaded and rendered without issue
//
// this is mostly to double-check that a configuration/library change hasn't
// bricked the documentation models
TEST(DocumentationModels, CanAllBeLoadedAndInitializedWithoutThrowingAnException)
{
    GloballyInitOpenSim();
    GloballyAddDirectoryToOpenSimGeometrySearchPath(std::filesystem::path{OSC_RESOURCES_DIR} / "geometry");

    SceneCache cache;
    OpenSimDecorationOptions options;

    std::filesystem::path docSourcesDir{OSC_DOCS_SOURCES_DIR};
    for_each_file_with_extensions_recursive(docSourcesDir, [&cache, &options](std::filesystem::path osim)
    {
        // load + initialize the documentation model
        UndoableModelStatePair model{osim};

        // try to generate 3D decorations from the model, which forces the backend
        // to (e.g.) try and load mesh files, etc.
        std::vector<SceneDecoration> decorations;
        GenerateModelDecorations(cache, model.getModel(), model.getState(), options, 1.0f, [&decorations](const OpenSim::Component&, SceneDecoration&& decoration)
        {
            decorations.push_back(decoration);
        });

        ASSERT_FALSE(decorations.empty());
    }, std::to_array<std::string_view>({".osim"}));
}

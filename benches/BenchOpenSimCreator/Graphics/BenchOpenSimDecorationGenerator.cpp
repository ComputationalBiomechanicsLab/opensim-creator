#include <OpenSimCreator/Graphics/OpenSimDecorationGenerator.h>

#include <benchmark/benchmark.h>
#include <OpenSim/Actuators/RegisterTypes_osimActuators.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSimCreator/Graphics/OpenSimDecorationOptions.h>
#include <OpenSimCreator/Platform/OpenSimCreatorApp.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>
#include <oscar/Graphics/Scene/SceneCache.h>
#include <oscar/Graphics/Scene/SceneDecoration.h>
#include <oscar/Platform/AppConfig.h>
#include <Simbody.h>

#include <filesystem>
#include <memory>

static void BM_OpenSimRenderRajagopalDecorations(benchmark::State& state)
{
    RegisterTypes_osimActuators();
    auto config = osc::LoadOpenSimCreatorConfig();
    std::filesystem::path modelPath = config.resource_directory() / "models" / "RajagopalModel" / "Rajagopal2015.osim";
    OpenSim::Model model{modelPath.string()};
    osc::InitializeModel(model);
    SimTK::State const& modelState = osc::InitializeState(model);

    osc::SceneCache meshCache;
    osc::OpenSimDecorationOptions decorationOptions;
    std::function<void(OpenSim::Component const&, osc::SceneDecoration&&)> outputFunc = [](OpenSim::Component const&, osc::SceneDecoration&&) {};


    // warmup
    osc::GenerateModelDecorations(meshCache, model, modelState, decorationOptions, 1.0, outputFunc);

    for ([[maybe_unused]] auto _ : state)
    {
        osc::GenerateModelDecorations(meshCache, model, modelState, decorationOptions, 1.0, outputFunc);
    }
}

BENCHMARK(BM_OpenSimRenderRajagopalDecorations)->Iterations(100000);

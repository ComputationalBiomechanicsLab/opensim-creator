#include "OpenSimCreator/Graphics/OpenSimDecorationGenerator.hpp"

#include "OpenSimCreator/Graphics/OpenSimDecorationOptions.hpp"
#include "OpenSimCreator/Utils/OpenSimHelpers.hpp"
#include "oscar/Graphics/MeshCache.hpp"
#include "oscar/Graphics/SceneDecoration.hpp"
#include "oscar/Platform/AppConfig.hpp"

#include <benchmark/benchmark.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Actuators/RegisterTypes_osimActuators.h>
#include <Simbody.h>

#include <filesystem>
#include <memory>

static void BM_OpenSimRenderRajagopalDecorations(benchmark::State& state)
{
    RegisterTypes_osimActuators();
    auto config = osc::AppConfig::load();
    std::filesystem::path modelPath = config->getResourceDir() / "models" / "RajagopalModel" / "Rajagopal2015.osim";
    OpenSim::Model model{modelPath.string()};
    osc::InitializeModel(model);
    SimTK::State const& modelState = osc::InitializeState(model);

    osc::MeshCache meshCache;
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

#include "src/OpenSimBindings/Graphics/OpenSimDecorationGenerator.hpp"

#include "src/Graphics/MeshCache.hpp"
#include "src/OpenSimBindings/Graphics/CustomDecorationOptions.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/Platform/Config.hpp"

#include <benchmark/benchmark.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Actuators/RegisterTypes_osimActuators.h>
#include <simbody.h>

#include <filesystem>
#include <memory>

static void BM_OpenSimRenderRajagopalDecorations(benchmark::State& state)
{
    RegisterTypes_osimActuators();
    auto config = osc::Config::load();
    std::filesystem::path modelPath = config->getResourceDir() / "models" / "RajagopalModel" / "Rajagopal2015.osim";
    OpenSim::Model model{modelPath.string()};
    osc::InitializeModel(model);
    SimTK::State const& modelState = osc::InitializeState(model);

    osc::MeshCache meshCache;
    osc::CustomDecorationOptions decorationOptions;
    std::function<void(OpenSim::Component const&, osc::SceneDecoration&&)> outputFunc = [](OpenSim::Component const&, osc::SceneDecoration&&) {};


    // warmup
    osc::GenerateModelDecorations(meshCache, model, modelState, decorationOptions, 1.0, outputFunc);

    for (auto _ : state)
    {
        osc::GenerateModelDecorations(meshCache, model, modelState, decorationOptions, 1.0, outputFunc);
    }
}

BENCHMARK(BM_OpenSimRenderRajagopalDecorations)->Iterations(100000);

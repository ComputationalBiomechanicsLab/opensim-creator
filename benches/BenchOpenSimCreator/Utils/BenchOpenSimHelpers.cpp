#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

#include <benchmark/benchmark.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>

#include <memory>

struct NestedComponentChain {
    std::unique_ptr<OpenSim::Component> root;
    OpenSim::Component const* deepestChild;
};

static NestedComponentChain GenerateNestedComponentChain()
{
    auto root = std::make_unique<OpenSim::PhysicalOffsetFrame>();
    root->setName("rootName");

    auto firstChild = std::make_unique<OpenSim::PhysicalOffsetFrame>();
    firstChild->setName("firstChild");

    auto secondChild = std::make_unique<OpenSim::PhysicalOffsetFrame>();
    secondChild->setName("secondChild");

    auto lastChild = std::make_unique<OpenSim::PhysicalOffsetFrame>();
    lastChild->setName("lastChild");

    OpenSim::Component* lastChildPtr = lastChild.get();

    secondChild->addComponent(lastChild.release());
    firstChild->addComponent(secondChild.release());
    root->addComponent(firstChild.release());

    return NestedComponentChain{std::move(root), lastChildPtr};
}

static void BM_OpenSimGetAbsolutePathString(benchmark::State& state)
{
    NestedComponentChain c = GenerateNestedComponentChain();
    for ([[maybe_unused]] auto _ : state)
    {
        benchmark::DoNotOptimize(c.deepestChild->getAbsolutePathString());
    }
}
BENCHMARK(BM_OpenSimGetAbsolutePathString);

static void BM_OscGetAbsolutePathString(benchmark::State& state)
{
    NestedComponentChain c = GenerateNestedComponentChain();
    for ([[maybe_unused]] auto _ : state)
    {
        benchmark::DoNotOptimize(osc::GetAbsolutePathString(*c.deepestChild));
    }
}
BENCHMARK(BM_OscGetAbsolutePathString);

static void BM_OscGetAbsolutePathStringAssigning(benchmark::State& state)
{
    NestedComponentChain c = GenerateNestedComponentChain();
    std::string out;
    for ([[maybe_unused]] auto _ : state)
    {
        osc::GetAbsolutePathString(*c.deepestChild, out);
    }
}
BENCHMARK(BM_OscGetAbsolutePathStringAssigning);

static void BM_OpenSimGetAbsolutePath(benchmark::State& state)
{
    NestedComponentChain c = GenerateNestedComponentChain();
    for ([[maybe_unused]] auto _ : state)
    {
        benchmark::DoNotOptimize(c.deepestChild->getAbsolutePath());
    }
}
BENCHMARK(BM_OpenSimGetAbsolutePath);

static void BM_OscGetAbsolutePath(benchmark::State& state)
{
    NestedComponentChain c = GenerateNestedComponentChain();
    for ([[maybe_unused]] auto _ : state)
    {
        benchmark::DoNotOptimize(osc::GetAbsolutePath(*c.deepestChild));
    }
}
BENCHMARK(BM_OscGetAbsolutePath);

#pragma once

#include <OpenSim/Common/Component.h>

#include <filesystem>

namespace osc::mow
{
    // abstract interface to a component that is capable of warping `n` other
    // components (`StrategyTargets`) during a model warp
    class ComponentWarpingStrategy : public OpenSim::Component {
    protected:
        ComponentWarpingStrategy() = default;
        ComponentWarpingStrategy(const ComponentWarpingStrategy&) = default;
        ComponentWarpingStrategy(ComponentWarpingStrategy&&) noexcept = default;
        ComponentWarpingStrategy& operator=(const ComponentWarpingStrategy&) = default;
        ComponentWarpingStrategy& operator=(ComponentWarpingStrategy&&) noexcept = default;
    public:
        ~ComponentWarpingStrategy() noexcept override = default;

        // StrategyTargets
    private:
    };

    // abstract interface to a component that is capable of warping `n`
    // `OpenSim::PhysicalOffsetFrame`s during a model warp
    class OffsetFrameWarpingStrategy : public ComponentWarpingStrategy {};

    // concrete implementation of an `OffsetFrameWarpingStrategy` in which
    // only the `translation` property of the offset frame is warped but the
    // rotation is left as-is
    class ThinPlateSplineOnlyTranslationOffsetFrameWarpingStrategy final : public OffsetFrameWarpingStrategy {
    public:
        // PointSources
    private:
    };

    // concrete implementation of an `OffsetFrameWarpingStrategy` in which the
    // implementation should produce a halting error rather than continuing with
    // the model warp
    class ProduceErrorOffsetFrameWarpingStrategy final : public OffsetFrameWarpingStrategy {
    };

    // concrete implementation of an `OffsetFrameWarpingStrategy` in which the
    // implementation should use a least-squares fit of the correspondences between
    // source/destination landmarks (`PointsToFit`) to compute the resulting offset
    // frame's `translation` and `rotation`
    class LeastSquaresOffsetFrameWarpingStrategy final : public OffsetFrameWarpingStrategy {
    public:
        // PointsToFit
    };

    // abstract interface to a component that is capable of warping `n`
    // `OpenSim::Station`s during a model warp
    class StationWarpingStrategy : public OpenSim::Component {};

    // concrete implementation of a `StationWarpingStrategy` that uses the Thin-Plate
    // Spline (TPS) algorithm to fit correspondences between mesh landmarks (`MeshSources`)
    class ThinPlateSplineStationWarpingStrategy final : public StationWarpingStrategy {
        // MeshSources
    };

    // TODO:
    // MuscleParameterWarpingStrategies
    //     MuscleParameterWarpingStrategy
    //         some_scaling_param
    //     IdentityMuscleParameterWarpingStrategy
    //
    // MeshWarpingStrategies
    //     ThinPlateSplineMeshWarpingStrategy
    //
    // WrapSurfaceWarpingStrategies
    //     WrapSurfaceWarpingStrategy
    //         LeastSquaresProjectionWrapSurfaceWarpingStrategy?

    // top-level model warping configuration file
    class ModelWarperConfiguration final : public OpenSim::Component {
        OpenSim_DECLARE_CONCRETE_OBJECT(ModelWarperConfiguration, OpenSim::Component);
    public:
        // constructs a blank (default) configuration object
        ModelWarperConfiguration();

        // constructs a `ModelWarperConfiguration` by loading its properties from an XML file
        // at the given filesystem location
        explicit ModelWarperConfiguration(const std::filesystem::path& filePath);
    private:
        void constructProperties();
    };
}
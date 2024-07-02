#pragma once

#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <OpenSim/Simulation/Model/Station.h>

#include <concepts>
#include <filesystem>
#include <sstream>
#include <string_view>
#include <typeinfo>
#include <unordered_set>
#include <utility>

namespace osc::mow
{
    // abstract interface to a component that is capable of warping `n` other
    // components (`StrategyTargets`) during a model warp
    class ComponentWarpingStrategy : public OpenSim::Component {
        OpenSim_DECLARE_ABSTRACT_OBJECT(ComponentWarpingStrategy, OpenSim::Component);
    public:
        OpenSim_DECLARE_LIST_PROPERTY(StrategyTargets, std::string, "a sequence of strategy target strings that this strategy applies to");
    protected:
        ComponentWarpingStrategy(const std::type_info& targetComponentType) :
            _targetComponentType{&targetComponentType}
        {
            constructProperty_StrategyTargets();
        }
        ComponentWarpingStrategy(const ComponentWarpingStrategy&) = default;
        ComponentWarpingStrategy(ComponentWarpingStrategy&&) noexcept = default;
        ComponentWarpingStrategy& operator=(const ComponentWarpingStrategy&) = default;
        ComponentWarpingStrategy& operator=(ComponentWarpingStrategy&&) noexcept = default;
    public:
        ~ComponentWarpingStrategy() noexcept override = default;

        const std::type_info& getTargetComponentTypeInfo() const
        {
            return *_targetComponentType;
        }
    private:
        void extendFinalizeFromProperties() override
        {
            assertStrategyTargetsNotEmpty();
            assertStrategyTargetsAreUnique();
        }

        void assertStrategyTargetsNotEmpty()
        {
            if (getProperty_StrategyTargets().empty()) {
                OPENSIM_THROW_FRMOBJ(OpenSim::Exception, "The <StrategyTargets> property of this component must be populated with at least one entry");
            }
        }

        void assertStrategyTargetsAreUnique()
        {
            const int numStrategyTargets = getProperty_StrategyTargets().size();
            std::unordered_set<std::string_view> uniqueStrategyTargets;
            uniqueStrategyTargets.reserve(numStrategyTargets);
            for (int i = 0; i < numStrategyTargets; ++i) {
                const std::string& strategyTarget = get_StrategyTargets(i);
                const auto [_, inserted] = uniqueStrategyTargets.emplace(strategyTarget);
                if (not inserted) {
                    std::stringstream ss;
                    ss << strategyTarget << ": duplicate strategy target detected: all strategy targets must be unique";
                    OPENSIM_THROW_FRMOBJ(OpenSim::Exception, std::move(ss).str());
                }
            }
        }

        const std::type_info* _targetComponentType;
    };

    // abstract interface to a component that is capable of warping `n` other
    // components (`StrategyTargets`) of type `T` during a model warp
    template<std::derived_from<OpenSim::Component> T>
    class ComponentWarpingStrategyFor : public ComponentWarpingStrategy {
    public:
        ComponentWarpingStrategyFor() : ComponentWarpingStrategy{typeid(T)} {}
    };

    // abstract interface to a component that is capable of warping `n`
    // `OpenSim::PhysicalOffsetFrame`s during a model warp
    class OffsetFrameWarpingStrategy : public ComponentWarpingStrategyFor<OpenSim::PhysicalOffsetFrame> {
        OpenSim_DECLARE_ABSTRACT_OBJECT(OffsetFrameWarpingStrategy, ComponentWarpingStrategyFor<OpenSim::PhysicalOffsetFrame>);
    };

    // concrete implementation of an `OffsetFrameWarpingStrategy` in which
    // only the `translation` property of the offset frame is warped but the
    // rotation is left as-is
    class ThinPlateSplineOnlyTranslationOffsetFrameWarpingStrategy final : public OffsetFrameWarpingStrategy {
        OpenSim_DECLARE_CONCRETE_OBJECT(ThinPlateSplineOnlyTranslationOffsetFrameWarpingStrategy, OffsetFrameWarpingStrategy);
    public:
        // PointSources
    private:
    };

    // concrete implementation of an `OffsetFrameWarpingStrategy` in which the
    // implementation should produce a halting error rather than continuing with
    // the model warp
    class ProduceErrorOffsetFrameWarpingStrategy final : public OffsetFrameWarpingStrategy {
        OpenSim_DECLARE_CONCRETE_OBJECT(ProduceErrorOffsetFrameWarpingStrategy, OffsetFrameWarpingStrategy);
    };

    // concrete implementation of an `OffsetFrameWarpingStrategy` in which the implementation
    // simply copies the `translation` and `rotation` of the source `OpenSim::PhysicalOffsetFrame` to
    // the destination model with no modifications
    class IdentityOffsetFrameWarpingStrategy final : public OffsetFrameWarpingStrategy {
        OpenSim_DECLARE_CONCRETE_OBJECT(IdentityOffsetFrameWarpingStrategy, OffsetFrameWarpingStrategy);
    };

    // abstract interface to a component that is capable of warping `n`
    // `OpenSim::Station`s during a model warp
    class StationWarpingStrategy : public ComponentWarpingStrategyFor<OpenSim::Station> {
        OpenSim_DECLARE_ABSTRACT_OBJECT(StationWarpingStrategy, ComponentWarpingStrategy);
    };

    // concrete implementation of a `StationWarpingStrategy` that uses the Thin-Plate
    // Spline (TPS) algorithm to fit correspondences between mesh landmarks (`MeshSources`)
    class ThinPlateSplineStationWarpingStrategy final : public StationWarpingStrategy {
        OpenSim_DECLARE_CONCRETE_OBJECT(ThinPlateSplineStationWarpingStrategy, StationWarpingStrategy);
        // MeshSources
    };

    // concrete implementation of a `StationWarpingStrategy` in which the implementation should
    // produce a halting error rather than continuing with the model warp
    class ProduceErrorStationWarpingStrategy final : public StationWarpingStrategy {
        OpenSim_DECLARE_CONCRETE_OBJECT(ProduceErrorStationWarpingStrategy, StationWarpingStrategy);
    };

    // concrete implementation of a `StationWarpingStrategy` in which the implementation should
    // just copy the station's postion (+parent) without any modification
    class IdentityStationWarpingStrategy final : public StationWarpingStrategy {
        OpenSim_DECLARE_CONCRETE_OBJECT(IdentityStationWarpingStrategy, StationWarpingStrategy);
    };

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
        void extendFinalizeFromProperties() override;
    };
}

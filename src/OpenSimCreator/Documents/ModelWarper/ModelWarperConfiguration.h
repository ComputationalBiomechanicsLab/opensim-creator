#pragma once

#include <OpenSimCreator/Documents/ModelWarper/ICloneable.h>
#include <OpenSimCreator/Documents/ModelWarper/IValidateable.h>
#include <OpenSimCreator/Documents/ModelWarper/IWarpDetailProvider.h>

#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <OpenSim/Simulation/Model/Station.h>

#include <compare>
#include <concepts>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeinfo>
#include <unordered_set>
#include <utility>

namespace osc::mow
{
    // describes how closely (if at all) a `ComponentWarpingStrategy` matches a
    // given `OpenSim::Component`
    //
    // used for resolving potentially-ambiguous matches across multiple strategies
    class StrategyMatchQuality final {
    public:
        static constexpr StrategyMatchQuality none() { return StrategyMatchQuality{State::None}; }
        static constexpr StrategyMatchQuality wildcard() { return StrategyMatchQuality{State::Wildcard}; }
        static constexpr StrategyMatchQuality exact() { return StrategyMatchQuality{State::Exact}; }

        constexpr operator bool () const { return _state != State::None; }

        friend constexpr bool operator==(StrategyMatchQuality, StrategyMatchQuality) = default;
        friend constexpr auto operator<=>(StrategyMatchQuality, StrategyMatchQuality) = default;
    private:
        enum class State {
            None,
            Wildcard,
            Exact
        };

        explicit constexpr StrategyMatchQuality(State state) :
            _state{state}
        {}

        State _state = State::None;
    };

    // additional warping parameters that are provided at runtime by the caller (usually, these are
    // less "static" than the parameters provided via the `ModelWarperConfiguration`)
    class RuntimeWarpParameters final {
    public:
        RuntimeWarpParameters() = default;
        explicit RuntimeWarpParameters(float blendFactor) : m_BlendFactor{blendFactor} {}

        float getBlendFactor() const { return m_BlendFactor; }
    private:
        float m_BlendFactor = 1.0f;
    };

    // an interface to an object that is capable of warping one specific component in the input model
    //
    // a `ComponentWarpingStrategy` produces this after matching the component, validating it against,
    // the rest of the model, etc.
    class IComponentWarper {
    protected:
        IComponentWarper() = default;
        IComponentWarper(const IComponentWarper&) = default;
        IComponentWarper(IComponentWarper&&) noexcept = default;
        IComponentWarper& operator=(const IComponentWarper&) = default;
        IComponentWarper& operator=(IComponentWarper&&) noexcept = default;
    public:
        virtual ~IComponentWarper() noexcept = default;

        void warpInPlace(
            const RuntimeWarpParameters& warpParameters,
            const WarpableModel& sourceModel,
            const OpenSim::Component& sourceComponent,
            OpenSim::Model& targetModel,
            OpenSim::Component& targetComponent)
        {
            implWarpInPlace(warpParameters, sourceModel, sourceComponent, targetModel, targetComponent);
        }
    private:
        virtual void implWarpInPlace(
            const RuntimeWarpParameters& warpParameters,
            const WarpableModel& sourceModel,
            const OpenSim::Component& sourceComponent,
            OpenSim::Model& targetModel,
            OpenSim::Component& targetComponent
        ) = 0;
    };

    // concrete implementation of an `IComponentWarper` that does nothing
    //
    // (handy as a stand-in during development)
    class IdentityComponentWarper : public IComponentWarper {
    private:
        void implWarpInPlace(
            const RuntimeWarpParameters&,
            const WarpableModel&,
            const OpenSim::Component&,
            OpenSim::Model&,
            OpenSim::Component&) override
        {}
    };

    // concrete implementation of an `IComponentWarper` that throws an exception
    // when used
    class ExceptionThrowingComponentWarper : public IComponentWarper {
    public:
        explicit ExceptionThrowingComponentWarper(std::string message) :
            m_Message{std::move(message)}
        {}
    private:
        void implWarpInPlace(
            const RuntimeWarpParameters&,
            const WarpableModel&,
            const OpenSim::Component&,
            OpenSim::Model&,
            OpenSim::Component&) override
        {
            throw std::runtime_error{m_Message};
        }

        std::string m_Message;
    };

    // abstract interface to a component that is capable of warping `n` other
    // components (`StrategyTargets`) during a model warp
    class ComponentWarpingStrategy :
        public OpenSim::Component,
        public ICloneable<ComponentWarpingStrategy>,
        public IWarpDetailProvider,
        public IValidateable {

        OpenSim_DECLARE_ABSTRACT_OBJECT(ComponentWarpingStrategy, OpenSim::Component)
    public:
        OpenSim_DECLARE_LIST_PROPERTY(StrategyTargets, std::string, "a sequence of strategy target strings that this strategy applies to");
    protected:
        ComponentWarpingStrategy()
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
            return implGetTargetComponentTypeInfo();
        }

        StrategyMatchQuality calculateMatchQuality(const OpenSim::Component& candidateComponent) const
        {
            if (not implIsMatchForComponentType(candidateComponent)) {
                return StrategyMatchQuality::none();
            }

            const auto componentAbsPath = candidateComponent.getAbsolutePathString();

            // loop through strategy targets and select the best one, throw if any match
            // is ambiguous
            StrategyMatchQuality best = StrategyMatchQuality::none();
            for (int i = 0; i < getProperty_StrategyTargets().size(); ++i) {
                const std::string& target = get_StrategyTargets(i);
                if (target == componentAbsPath) {
                    // you can't do any better than this, and `extendFinalizeFromProperties`
                    // guarantees no other `StrategyTarget`s are going to match exactly, so
                    // exit early
                    return StrategyMatchQuality::exact();
                }
                else if (target == "*") {
                    best = StrategyMatchQuality::wildcard();
                }
            }
            return best;
        }

        std::unique_ptr<IComponentWarper> createWarper(const WarpableModel& model, const OpenSim::Component& component)
        {
            return implCreateWarper(model, component);
        }
    private:
        virtual const std::type_info& implGetTargetComponentTypeInfo() const = 0;
        virtual bool implIsMatchForComponentType(const OpenSim::Component&) const = 0;
        virtual std::unique_ptr<IComponentWarper> implCreateWarper(const WarpableModel&, const OpenSim::Component&) = 0;

        void extendFinalizeFromProperties() override
        {
            assertStrategyTargetsNotEmpty();
            assertStrategyTargetsAreUnique();
        }

        void assertStrategyTargetsNotEmpty() const
        {
            if (getProperty_StrategyTargets().empty()) {
                OPENSIM_THROW_FRMOBJ(OpenSim::Exception, "The <StrategyTargets> property of this component must be populated with at least one entry");
            }
        }

        void assertStrategyTargetsAreUnique() const
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
    };

    // abstract interface to a component that is capable of warping `n` other
    // components (`StrategyTargets`) of type `T` during a model warp
    template<std::derived_from<OpenSim::Component> T>
    class ComponentWarpingStrategyFor : public ComponentWarpingStrategy {
    public:
        ComponentWarpingStrategyFor() = default;

    private:
        const std::type_info& implGetTargetComponentTypeInfo() const override
        {
            return typeid(T);
        }

        bool implIsMatchForComponentType(const OpenSim::Component& component) const override
        {
            return dynamic_cast<const T*>(&component) != nullptr;
        }
    };

    // abstract interface to a component that is capable of warping `n`
    // `OpenSim::PhysicalOffsetFrame`s during a model warp
    class OffsetFrameWarpingStrategy : public ComponentWarpingStrategyFor<OpenSim::PhysicalOffsetFrame> {
        OpenSim_DECLARE_ABSTRACT_OBJECT(OffsetFrameWarpingStrategy, ComponentWarpingStrategyFor<OpenSim::PhysicalOffsetFrame>)
    };

    // concrete implementation of an `OffsetFrameWarpingStrategy` in which
    // only the `translation` property of the offset frame is warped but the
    // rotation is left as-is
    class ThinPlateSplineOnlyTranslationOffsetFrameWarpingStrategy final : public OffsetFrameWarpingStrategy {
        OpenSim_DECLARE_CONCRETE_OBJECT(ThinPlateSplineOnlyTranslationOffsetFrameWarpingStrategy, OffsetFrameWarpingStrategy)
    public:
        // PointSources
    private:
        std::unique_ptr<ComponentWarpingStrategy> implClone() const override
        {
            return std::make_unique<ThinPlateSplineOnlyTranslationOffsetFrameWarpingStrategy>(*this);
        }

        std::vector<WarpDetail> implWarpDetails() const override
        {
            // TODO: similar steps to `ThinPlateSplineStationWarpingStrategy`
            return {};
        }

        std::unique_ptr<IComponentWarper> implCreateWarper(const WarpableModel&, const OpenSim::Component&) override
        {
            return std::make_unique<IdentityComponentWarper>();
        }
    };

    // concrete implementation of an `OffsetFrameWarpingStrategy` in which the
    // implementation should produce a halting error rather than continuing with
    // the model warp
    class ProduceErrorOffsetFrameWarpingStrategy final : public OffsetFrameWarpingStrategy {
        OpenSim_DECLARE_CONCRETE_OBJECT(ProduceErrorOffsetFrameWarpingStrategy, OffsetFrameWarpingStrategy)
    private:
        std::unique_ptr<ComponentWarpingStrategy> implClone() const override
        {
            return std::make_unique<ProduceErrorOffsetFrameWarpingStrategy>(*this);
        }

        std::vector<WarpDetail> implWarpDetails() const override
        {
            return {
                WarpDetail{"description", "this warping strategy will always produce an error: you probably need to configure a better strategy for this component"},
            };
        }

        std::unique_ptr<IComponentWarper> implCreateWarper(const WarpableModel&, const OpenSim::Component&) override
        {
            return std::make_unique<ExceptionThrowingComponentWarper>("ProduceErrorStationWarpingStrategy: TODO: configuration-customizable error message");
        }

        std::vector<ValidationCheckResult> implValidate(const WarpableModel&) const
        {
            return {
                ValidationCheckResult{"this warping strategy always produces an error (TODO: configuration-customizable error message)", ValidationCheckState::Error},
            };
        }
    };

    // concrete implementation of an `OffsetFrameWarpingStrategy` in which the implementation
    // simply copies the `translation` and `rotation` of the source `OpenSim::PhysicalOffsetFrame` to
    // the destination model with no modifications
    class IdentityOffsetFrameWarpingStrategy final : public OffsetFrameWarpingStrategy {
        OpenSim_DECLARE_CONCRETE_OBJECT(IdentityOffsetFrameWarpingStrategy, OffsetFrameWarpingStrategy)
    private:
        std::unique_ptr<ComponentWarpingStrategy> implClone() const override
        {
            return std::make_unique<IdentityOffsetFrameWarpingStrategy>(*this);
        }

        std::vector<WarpDetail> implWarpDetails() const override
        {
            return {
                WarpDetail{"description", "this warping strategy will leave the frame untouched"},
            };
        }

        std::unique_ptr<IComponentWarper> implCreateWarper(const WarpableModel&, const OpenSim::Component&) override
        {
            return std::make_unique<IdentityComponentWarper>();
        }

        std::vector<ValidationCheckResult> implValidate(const WarpableModel&) const
        {
            return {
                ValidationCheckResult{"this is an identity warp (i.e. it ignores warping this offset frame altogether)", ValidationCheckState::Warning},
            };
        }
    };

    // abstract interface to a component that is capable of warping `n`
    // `OpenSim::Station`s during a model warp
    class StationWarpingStrategy : public ComponentWarpingStrategyFor<OpenSim::Station> {
        OpenSim_DECLARE_ABSTRACT_OBJECT(StationWarpingStrategy, ComponentWarpingStrategy)
    };

    // concrete implementation of a `StationWarpingStrategy` that uses the Thin-Plate
    // Spline (TPS) algorithm to fit correspondences between mesh landmarks (`MeshSources`)
    class ThinPlateSplineStationWarpingStrategy final : public StationWarpingStrategy {
        OpenSim_DECLARE_CONCRETE_OBJECT(ThinPlateSplineStationWarpingStrategy, StationWarpingStrategy)
        // MeshSources
    private:
        std::unique_ptr<ComponentWarpingStrategy> implClone() const override
        {
            return std::make_unique<ThinPlateSplineStationWarpingStrategy>(*this);
        }

        std::vector<ValidationCheckResult> implValidate(const WarpableModel&) const
        {
            /*
            * std::vector<ValidationCheckResult> rv;

            // has a source landmarks file
            {
                std::stringstream ss;
                ss << "has source landmarks file at " << recommendedSourceLandmarksFilepath().string();
                rv.emplace_back(std::move(ss).str(), hasSourceLandmarksFilepath());
            }

            // has source landmarks
            rv.emplace_back("source landmarks file contains landmarks", hasSourceLandmarks());

            // has destination mesh file
            {
                std::stringstream ss;
                ss << "has destination mesh file at " << recommendedDestinationMeshFilepath().string();
                rv.emplace_back(std::move(ss).str(), hasDestinationMeshFilepath());
            }

            // has destination landmarks file
            {
                std::stringstream ss;
                ss << "has destination landmarks file at " << recommendedDestinationLandmarksFilepath().string();
                rv.emplace_back(std::move(ss).str(), hasDestinationLandmarksFilepath());
            }

            // has destination landmarks
            rv.emplace_back("destination landmarks file contains landmarks", hasDestinationLandmarks());

            // has at least a few paired landmarks
            rv.emplace_back("at least three landmarks can be paired between source/destination", getNumFullyPairedLandmarks() >= 3);

            // (warning): has no unpaired landmarks
            rv.emplace_back("there are no unpaired landmarks", getNumUnpairedLandmarks() == 0 ? ValidationCheckState::Ok : ValidationCheckState::Warning);
            */
            return {};
        }

        std::vector<WarpDetail> implWarpDetails() const override
        {
            /*
            * std::vector<WarpDetail> rv;
            rv.emplace_back("source mesh filepath", getSourceMeshAbsoluteFilepath().string());
            rv.emplace_back("source landmarks expected filepath", recommendedSourceLandmarksFilepath().string());
            rv.emplace_back("has source landmarks file?", hasSourceLandmarksFilepath() ? "yes" : "no");
            rv.emplace_back("number of source landmarks", std::to_string(getNumSourceLandmarks()));
            rv.emplace_back("destination mesh expected filepath", recommendedDestinationMeshFilepath().string());
            rv.emplace_back("has destination mesh?", hasDestinationMeshFilepath() ? "yes" : "no");
            rv.emplace_back("destination landmarks expected filepath", recommendedDestinationLandmarksFilepath().string());
            rv.emplace_back("has destination landmarks file?", hasDestinationLandmarksFilepath() ? "yes" : "no");
            rv.emplace_back("number of destination landmarks", std::to_string(getNumDestinationLandmarks()));
            rv.emplace_back("number of paired landmarks", std::to_string(getNumFullyPairedLandmarks()));
            rv.emplace_back("number of unpaired landmarks", std::to_string(getNumUnpairedLandmarks()));
            return rv;
            */
            return {};
        }

        std::unique_ptr<IComponentWarper> implCreateWarper(const WarpableModel&, const OpenSim::Component&) override
        {
            return std::make_unique<IdentityComponentWarper>();
        }
    };

    // concrete implementation of a `StationWarpingStrategy` in which the implementation should
    // produce a halting error rather than continuing with the model warp
    class ProduceErrorStationWarpingStrategy final : public StationWarpingStrategy {
        OpenSim_DECLARE_CONCRETE_OBJECT(ProduceErrorStationWarpingStrategy, StationWarpingStrategy)
    private:
        std::unique_ptr<ComponentWarpingStrategy> implClone() const override
        {
            return std::make_unique<ProduceErrorStationWarpingStrategy>(*this);
        }

        std::vector<WarpDetail> implWarpDetails() const override
        {
            return {
                WarpDetail{"description", "this warping strategy will always produce an error: you probably need to configure a better strategy for this component"},
            };
        }

        std::unique_ptr<IComponentWarper> implCreateWarper(const WarpableModel&, const OpenSim::Component&) override
        {
            return std::make_unique<ExceptionThrowingComponentWarper>("ProduceErrorStationWarpingStrategy: TODO: configuration-customizable error message");
        }

        std::vector<ValidationCheckResult> implValidate(const WarpableModel&) const
        {
            return {
                ValidationCheckResult{"this warping strategy always produces an error (TODO: configuration-customizable error message)", ValidationCheckState::Error},
            };
        }
    };

    // concrete implementation of a `StationWarpingStrategy` in which the implementation should
    // just copy the station's postion (+parent) without any modification
    class IdentityStationWarpingStrategy final : public StationWarpingStrategy {
        OpenSim_DECLARE_CONCRETE_OBJECT(IdentityStationWarpingStrategy, StationWarpingStrategy)
    private:
        std::unique_ptr<ComponentWarpingStrategy> implClone() const override
        {
            return std::make_unique<IdentityStationWarpingStrategy>(*this);
        }

        std::vector<WarpDetail> implWarpDetails() const override
        {
            return {
                WarpDetail{"description", "this warping strategy will leave the station untouched"},
            };
        }

        std::unique_ptr<IComponentWarper> implCreateWarper(const WarpableModel&, const OpenSim::Component&) override
        {
            return std::make_unique<IdentityComponentWarper>();
        }

        std::vector<ValidationCheckResult> implValidate(const WarpableModel&) const
        {
            return {
                ValidationCheckResult{"this is an identity warp (i.e. it ignores warping this station altogether)", ValidationCheckState::Warning},
            };
        }
    };

    // top-level model warping configuration file
    class ModelWarperConfiguration final : public OpenSim::Component {
        OpenSim_DECLARE_CONCRETE_OBJECT(ModelWarperConfiguration, OpenSim::Component)
    public:
        // constructs a blank (default) configuration object
        ModelWarperConfiguration();

        // constructs a `ModelWarperConfiguration` by loading its properties from an XML file
        // at the given filesystem location
        explicit ModelWarperConfiguration(const std::filesystem::path& filePath);

        const ComponentWarpingStrategy* tryMatchStrategy(const OpenSim::Component&) const;
    private:
        void constructProperties();
        void extendFinalizeFromProperties() override;
    };
}

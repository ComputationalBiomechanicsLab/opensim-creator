#pragma once

#include <OpenSimCreator/Documents/ModelWarper/ICloneable.h>
#include <OpenSimCreator/Documents/ModelWarper/IValidateable.h>
#include <OpenSimCreator/Documents/ModelWarper/IWarpDetailProvider.h>
#include <OpenSimCreator/Utils/LandmarkPair3D.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <OpenSim/Simulation/Model/Station.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/CopyOnUpdPtr.h>

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
#include <vector>

namespace osc::mow
{
    // describes how closely, if at all, a `ComponentWarpingStrategy` matches a
    // given `OpenSim::Component`
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

    // parameters that are provided each time a model warp is requested at runtime
    class RuntimeWarpParameters final {
    public:
        RuntimeWarpParameters() = default;
        explicit RuntimeWarpParameters(float blendFactor) : m_BlendFactor{blendFactor} {}

        float getBlendFactor() const { return m_BlendFactor; }
    private:
        float m_BlendFactor = 1.0f;
    };

    // an associative cache that can be used to fetch relevant warping state
    //
    // higher-level systems should try persist this cache between components,
    // `IComponentWarper`s, `ComponentWarpingStrategy`s, and model-warping
    // requests (e.g. after a user edit in a UI) to minimize redundant work
    class WarpCache final {
    };

    // an abstract interface to something that is capable of warping an `OpenSim::Component`
    // in an `OpenSim::Model`
    //
    // this is produced by matching a `ComponentWarpingStrategy` to a specific `OpenSim::Component`
    class IComponentWarper {
    protected:
        IComponentWarper() = default;
        IComponentWarper(const IComponentWarper&) = default;
        IComponentWarper(IComponentWarper&&) noexcept = default;
        IComponentWarper& operator=(const IComponentWarper&) = default;
        IComponentWarper& operator=(IComponentWarper&&) noexcept = default;
    public:
        virtual ~IComponentWarper() noexcept = default;

        // warps `targetComponent` in `targetModel` in-place, assumes `sourceModel` and
        // `sourceComponent` are equivalent to the model's + component's pre-warp state
        void warpInPlace(
            const RuntimeWarpParameters& warpParameters,
            WarpCache& warpCache,
            const OpenSim::Model& sourceModel,
            const OpenSim::Component& sourceComponent,
            OpenSim::Model& targetModel,
            OpenSim::Component& targetComponent)
        {
            implWarpInPlace(
                warpParameters,
                warpCache,
                sourceModel,
                sourceComponent,
                targetModel,
                targetComponent
            );
        }
    private:

        // overriders should:
        //
        // - mutate the `targetComponent` based on the warping behavior of their concrete implementation
        // - handle the `RuntimeWarpParameters` appropriately
        // - try to use `WarpCache` as much as possible (performance)
        //
        // throw a `std::exception` if there's an error (e.g. `targetComponent` cannot be warped, bad properties)
        virtual void implWarpInPlace(
            const RuntimeWarpParameters& warpParameters,
            WarpCache& warpCache,
            const OpenSim::Model& sourceModel,
            const OpenSim::Component& sourceComponent,
            OpenSim::Model& targetModel,
            OpenSim::Component& targetComponent
        ) = 0;
    };

    // an `IComponentWarper` that leaves the target `OpenSim::Component` untouched
    //
    // this can be useful for development, or for when the type of the component
    // isn't really warp-able (e.g. frame geometry, `OpenSim::Controller`s, etc.)
    class IdentityComponentWarper final : public IComponentWarper {
    private:
        void implWarpInPlace(
            const RuntimeWarpParameters&,
            WarpCache&,
            const OpenSim::Model&,
            const OpenSim::Component&,
            OpenSim::Model&,
            OpenSim::Component&) final
        {
            // don't do anything (it's an identity warper)
        }
    };

    // an `IComponentWarper` that throws a `std::exception` with the given message when
    // warping is required
    //
    // this can be useful in the warping configuration file, so that users can express "if
    // this component matches, then it's an error"
    class ExceptionThrowingComponentWarper final : public IComponentWarper {
    public:
        ExceptionThrowingComponentWarper() = default;

        explicit ExceptionThrowingComponentWarper(std::string message) :
            m_Message{std::move(message)}
        {}
    private:
        void implWarpInPlace(
            const RuntimeWarpParameters&,
            WarpCache&,
            const OpenSim::Model&,
            const OpenSim::Component&,
            OpenSim::Model&,
            OpenSim::Component&) final
        {
            throw std::runtime_error{m_Message};
        }

        std::string m_Message = "(no error message available)";
    };

    // an abstract base class for an `OpenSim::Component` that is capable of matching
    // against, and producing `IComponentWarper`s for, components (`StrategyTargets`)
    // in the source model
    class ComponentWarpingStrategy :
        public OpenSim::Component,
        public ICloneable<ComponentWarpingStrategy>,
        public IWarpDetailProvider {

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
                return StrategyMatchQuality::none();  // mis-matched implementation, this will never match
            }

            // select the best (max) match of all available possibilities
            const auto componentAbsPath = candidateComponent.getAbsolutePathString();
            StrategyMatchQuality best = StrategyMatchQuality::none();
            for (int i = 0; i < getProperty_StrategyTargets().size(); ++i) {
                const std::string& target = get_StrategyTargets(i);
                if (target == componentAbsPath) {
                    best = max(best, StrategyMatchQuality::exact());
                }
                else if (target == "*") {
                    best = max(best, StrategyMatchQuality::wildcard());
                }
            }
            return best;
        }

        std::unique_ptr<IComponentWarper> createWarper(
            const OpenSim::Model& model,
            const OpenSim::Component& component)
        {
            if (auto quality = calculateMatchQuality(component); quality <= StrategyMatchQuality::none()) {
                // the caller probably called this function without first checking `calculateMatchQuality`, so throw here
                std::stringstream msg;
                msg << component.getAbsolutePathString() << ": cannot be warped by " << getName() << "(type: " << getConcreteClassName() << ')';
                OPENSIM_THROW_FRMOBJ(OpenSim::Exception, std::move(msg).str());
            }

            // else: call into the concrete implementation
            return implCreateWarper(model, component);
        }
    private:
        // overriders should return the `typeinfo` of the concrete class that this warper can warp
        virtual const std::type_info& implGetTargetComponentTypeInfo() const = 0;

        // overriders should return `true` if `implCreateWarper` would create a valid warper for the given `OpenSim::Component`
        virtual bool implIsMatchForComponentType(const OpenSim::Component&) const = 0;

        // overriders should return a valid `IComponentWarper` that can warp the given `OpenSim::Component` at runtime
        virtual std::unique_ptr<IComponentWarper> implCreateWarper(const OpenSim::Model&, const OpenSim::Component&) = 0;

        // by default, returns an empty `std::vector<ValidationCheckResult>` (i.e. no validation checks made)
        //
        // overriders should return a `std::vector<ValidationCheckResult>` that describe any validation
        // checks (incl. `Ok`, `Warning` and `Error` checks) against the provided `OpenSim::Model`
        virtual std::vector<ValidationCheckResult> implValidate(const OpenSim::Model&) const { return {}; }

        void extendFinalizeFromProperties() override
        {
            assertStrategyTargetsNotEmpty();
            assertStrategyTargetsAreUnique();
        }

        // raises an `OpenSim::Exception` if the `StrategyTargets` property is empty
        void assertStrategyTargetsNotEmpty() const
        {
            if (getProperty_StrategyTargets().empty()) {
                OPENSIM_THROW_FRMOBJ(OpenSim::Exception, "The <StrategyTargets> property of this component must be populated with at least one entry");
            }
        }

        // raises an `OpenSim::Exception` if the `StrategyTargets` property contains duplicate entries
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

    // an abstract base class for a `ComponentWarpingStrategy` specialized for `T`
    template<std::derived_from<OpenSim::Component> T>
    class ComponentWarpingStrategyFor : public ComponentWarpingStrategy {
    public:
        ComponentWarpingStrategyFor() = default;

    private:
        const std::type_info& implGetTargetComponentTypeInfo() const final
        {
            return typeid(T);
        }

        bool implIsMatchForComponentType(const OpenSim::Component& component) const final
        {
            return dynamic_cast<const T*>(&component) != nullptr;
        }
    };

    // a `ComponentWarpingStrategy` that's specialized for `OpenSim::PhysicalOffsetFrame`s
    class OffsetFrameWarpingStrategy : public ComponentWarpingStrategyFor<OpenSim::PhysicalOffsetFrame> {
        OpenSim_DECLARE_ABSTRACT_OBJECT(OffsetFrameWarpingStrategy, ComponentWarpingStrategyFor<OpenSim::PhysicalOffsetFrame>)
    };

    // a `ComponentWarpingStrategy` that's specialized for `OpenSim::Station`s
    class StationWarpingStrategy : public ComponentWarpingStrategyFor<OpenSim::Station> {
        OpenSim_DECLARE_ABSTRACT_OBJECT(StationWarpingStrategy, ComponentWarpingStrategyFor<OpenSim::Station>)
    };

    // a `ComponentWarpingStrategy` that's specialized for `OpenSim::Mesh`es
    class MeshWarpingStrategy : public ComponentWarpingStrategyFor<OpenSim::Mesh> {
        OpenSim_DECLARE_ABSTRACT_OBJECT(MeshWarpingStrategy, ComponentWarpingStrategyFor<OpenSim::Mesh>)
    };

    // a sequence of paired (corresponding) landmarks expressed in a common base
    // `OpenSim::Frame`
    //
    // designed to be cheap to copy and compare, because this information might be
    // shared or cached by multiple systems
    class PairedPoints final {
    public:
        PairedPoints() = default;

        template<std::ranges::input_range Range>
        requires std::convertible_to<std::ranges::range_value_t<Range>, LandmarkPair3D>
        explicit PairedPoints(Range&& range, const OpenSim::ComponentPath& baseFrameAbsPath) :
            m_Data{make_cow<Data>(std::forward<Range>(range), baseFrameAbsPath)}
        {}

        auto begin() const { return m_Data->pointsInBaseFrame.begin(); }
        auto end() const { return m_Data->pointsInBaseFrame.end(); }

        const OpenSim::ComponentPath& getBaseFrameAbsPath() const { return m_Data->baseFrameAbsPath; }

        friend bool operator==(const PairedPoints& lhs, const PairedPoints& rhs)
        {
            return lhs.m_Data == rhs.m_Data or *lhs.m_Data == *rhs.m_Data;
        }
    private:
        struct Data final {
            Data() = default;

            template<std::ranges::input_range Range>
            requires std::convertible_to<std::ranges::range_value_t<Range>, LandmarkPair3D>
            explicit Data(Range&& range, const OpenSim::ComponentPath& baseFrameAbsPath_) :
                pointsInBaseFrame{std::ranges::begin(range), std::ranges::end(range)},
                baseFrameAbsPath{baseFrameAbsPath_}
            {}

            friend bool operator==(const Data&, const Data&) = default;

            std::vector<LandmarkPair3D> pointsInBaseFrame;
            OpenSim::ComponentPath baseFrameAbsPath;
        };
        CopyOnUpdPtr<Data> m_Data = make_cow<Data>();
    };

    // an abstract base class to an `OpenSim::Object` that can lookup and produce
    // `PairedPoints` (e.g. for feeding into a Thin-Plate Spline fitter)
    class PairedPointSource : public OpenSim::Object, public IWarpDetailProvider {
        OpenSim_DECLARE_ABSTRACT_OBJECT(PairedPointSource, OpenSim::Object)
    public:

        // returns the paired points, based on the concrete implementation's approach
        // for finding + pairing them
        //
        // throws a `std::exception` if the concrete implementation's `implValidate`
        // returns any `ValidationCheckResult` with a `state()` of `ValidationCheckState::Error`
        PairedPoints getPairedPoints(
            WarpCache& warpCache,
            const OpenSim::Model& sourceModel,
            const OpenSim::Component& sourceComponent
        );

        // returns a sequence of `ValidationCheckResult`s related to applying the provided
        // `sourceModel` and `sourceComponent` to this `PairedPointSource`
        std::vector<ValidationCheckResult> validate(
            const OpenSim::Model& sourceModel,
            const OpenSim::Component& sourceComponent) const
        {
            return implValidate(sourceModel, sourceComponent);
        }
    private:
        // overriders should find + pair the points and return a `PairedPoints` instance, or throw an exception
        virtual PairedPoints implGetPairedPoints(
            WarpCache& warpCache,
            const OpenSim::Model& sourceModel,
            const OpenSim::Component& sourceComponent
        ) = 0;

        // by default, returns no `ValidationCheckResult`s (i.e. no validation)
        //
        // overriders should return `ValidationCheckResult`s for their concrete `PairedPointSource`
        // implementation--including checks that pass/warn--so that the information can be propagated
        // to other layers of the system (e.g. so that a UI system could display "this thing is ok")
        virtual std::vector<ValidationCheckResult> implValidate(
            const OpenSim::Model& sourceModel,
            const OpenSim::Component& sourceComponent
        ) const;
    };

    // a `PairedPointsource` that uses heuristics to find the landmarks associated with one `OpenSim::Mesh`
    //
    // - the source component supplied must be an `OpenSim::Mesh`; otherwise, a validation error is generated
    // - the source landmarks file is assumed to be on the filesystem "next to" the `OpenSim::Mesh` and
    //   named `${mesh_file_name_without_extension}.landmarks.csv`; otherwise, a validation error is generated
    // - the destination landmarks file is assumed to be on the filesystem "next to" the `OpenSim::Model`
    //   in a directory named `DestinationGeometry` at `${model_parent_directory}/DestinationGeometry/${mesh_file_name_without_extension}.landmarks.csv`;
    //   otherwise, a validation error is generated
    // - else, accept those pairs as "the mesh's landmark pairs" (even if empty)
    class LandmarkPairsAssociatedWithMesh final : public PairedPointSource {
        OpenSim_DECLARE_CONCRETE_OBJECT(LandmarkPairsAssociatedWithMesh, PairedPointSource)
    private:
        PairedPoints implGetPairedPoints(WarpCache&, const OpenSim::Model&, const OpenSim::Component&) final
        {
            return {};  // TODO
        }

        std::vector<ValidationCheckResult> implValidate(
            const OpenSim::Model& sourceModel,
            const OpenSim::Component& sourceComponent) const final
        {
            std::vector<ValidationCheckResult> rv;

            const auto* sourceMesh = dynamic_cast<const OpenSim::Mesh*>(&sourceComponent);
            if (not sourceMesh) {
                std::stringstream ss;
                ss << sourceComponent.getName() << "(type: " << sourceComponent.getConcreteClassName() << ") is not an OpenSim::Mesh. " << getName() << "(type: " << getConcreteClassName() << ") requires this";
                rv.emplace_back(std::move(ss).str(), ValidationCheckState::Error);
                return rv;
            }

            const auto sourceMeshPath = FindGeometryFileAbsPath(sourceModel, *sourceMesh);
            if (not sourceMeshPath) {
                std::stringstream ss;
                ss << sourceComponent.getName() << ": the absolute filesystem location of this mesh cannot be found";
                rv.emplace_back(std::move(ss).str(), ValidationCheckState::Error);
                return rv;
            }
            else {
                std::stringstream ss;
                ss << sourceMesh->getName() << ": was found on the filesystem at " << *sourceMeshPath;
                rv.emplace_back(std::move(ss).str(), ValidationCheckState::Ok);
            }

            auto sourceLandmarksPath{*sourceMeshPath};
            sourceLandmarksPath.replace_extension(".landmarks.csv");
            if (not std::filesystem::exists(sourceLandmarksPath)) {
                std::stringstream ss;
                ss << sourceMesh->getName() << ": could not find an associated .landmarks.csv file at " << sourceLandmarksPath;
                rv.emplace_back(std::move(ss).str(), ValidationCheckState::Error);
                return rv;
            }
            else {
                std::stringstream ss;
                ss << sourceMesh->getName() << ": has a .landmarks.csv file at " << sourceLandmarksPath;
                rv.emplace_back(std::move(ss).str(), ValidationCheckState::Ok);
            }

            const auto modelFilePath = TryFindInputFile(sourceModel);
            if (not modelFilePath) {
                std::stringstream ss;
                ss << getName() << ": cannot find the supplied model file's filesystem location: this is required in order to locate the `DestinationGeometry` directory";
                rv.emplace_back(std::move(ss).str(), ValidationCheckState::Error);
                return rv;
            }
            else {
                std::stringstream ss;
                ss << getName() << ": the model file was found at " << *modelFilePath;
                rv.emplace_back(std::move(ss).str(), ValidationCheckState::Ok);
            }

            const auto destinationLandmarksPath = modelFilePath->parent_path() / "DestinationGeometry" / sourceMeshPath->filename().replace_extension(".landmarks.csv");
            if (not std::filesystem::exists(destinationLandmarksPath)) {
                std::stringstream ss;
                ss << sourceMesh->getName() << ": cannot find a destination .landmarks.csv at " << destinationLandmarksPath;
                rv.emplace_back(std::move(ss).str(), ValidationCheckState::Error);
                return rv;
            }
            else {
                std::stringstream ss;
                ss << getName() << ": found a destination .landmarks.csv file at " << destinationLandmarksPath;
                rv.emplace_back(std::move(ss).str(), ValidationCheckState::Ok);
            }

            return rv;
        }
    };

    // a `PairedPointSource` that uses heuristics to find the most appropriate `PairedPoints`
    // for a given `OpenSim::Component`. The heuristic is:
    //
    // 1. find the base frame of the component:
    //
    //     - `OpenSim::Station`s have a `parent_frame`
    //     - `OpenSim::PhysicalOffsetFrame`s have a `parent_frame`
    //     - `OpenSim::Mesh`es have a `parent_frame`
    //     - (etc. - this needs to be handled on a per-component-type basis)
    //
    // 2. find all `OpenSim::Mesh`es in the source model that are attached to the same base frame:
    //
    //     - if no `OpenSim::Mesh`es are attached to the base frame, throw an error
    //     - if more than one `OpenSim::Mesh` are attached to the base frame, throw an error
    //     - else, accept the resulting 1..n meshes as "the input mesh set"
    //
    // 3. for each mesh in "the input mesh set":
    //
    //     - extract their `PairedPoints` "as if" by using `LandmarksAttachedToSuppliedMesh` any errors
    //       should be propagated upwards
    //     - transform all of "the mesh's landmark pairs" in the mesh's frame to the base frame found in step 1
    //     - merge all of "the mesh's landmark pairs" in "the input mesh set" into a `PairedPoints`
    class LandmarkPairsOfMeshesAttachedToSameBaseFrame final : public PairedPointSource {
        OpenSim_DECLARE_CONCRETE_OBJECT(LandmarkPairsOfMeshesAttachedToSameBaseFrame, PairedPointSource)
    private:
        PairedPoints implGetPairedPoints(WarpCache&, const OpenSim::Model&, const OpenSim::Component&) final
        {
            return {};  // TODO
        }
    };

    // an `OffsetFrameWarpingStrategy` that uses point correspondences and Thin-Plate Spline (TPS) warping to
    // warp the `translation` property of an `OpenSim::PhysicalOffsetFrame`
    class ThinPlateSplineOnlyTranslationOffsetFrameWarpingStrategy final : public OffsetFrameWarpingStrategy {
        OpenSim_DECLARE_CONCRETE_OBJECT(ThinPlateSplineOnlyTranslationOffsetFrameWarpingStrategy, OffsetFrameWarpingStrategy)

    public:
        OpenSim_DECLARE_PROPERTY(point_source, PairedPointSource, "a `PairedPointSource` that describes where the Thin Plate Spline algorithm should source its data from");

    private:
        std::unique_ptr<ComponentWarpingStrategy> implClone() const final
        {
            return std::make_unique<ThinPlateSplineOnlyTranslationOffsetFrameWarpingStrategy>(*this);
        }

        std::vector<WarpDetail> implWarpDetails() const final
        {
            // TODO: similar steps to `ThinPlateSplineStationWarpingStrategy`
            return {};
        }

        std::unique_ptr<IComponentWarper> implCreateWarper(const OpenSim::Model&, const OpenSim::Component&) final
        {
            return std::make_unique<IdentityComponentWarper>();
        }
    };

    // an `OffsetFrameWarpingStrategy` that always produces an error
    //
    // usually used by configuration writers as a fallback to indicate "if you matched this far
    // then it's an error"
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

        std::unique_ptr<IComponentWarper> implCreateWarper(const OpenSim::Model&, const OpenSim::Component&) override
        {
            return std::make_unique<ExceptionThrowingComponentWarper>("ProduceErrorStationWarpingStrategy: TODO: configuration-customizable error message");
        }

        std::vector<ValidationCheckResult> implValidate(const OpenSim::Model&) const override
        {
            return {
                ValidationCheckResult{"this warping strategy always produces an error (TODO: configuration-customizable error message)", ValidationCheckState::Error},
            };
        }
    };

    // an `OffsetFrameWarpingStrategy` that leaves the `OpenSim::PhysicalOffsetFrame` untouched
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

        std::unique_ptr<IComponentWarper> implCreateWarper(const OpenSim::Model&, const OpenSim::Component&) override
        {
            return std::make_unique<IdentityComponentWarper>();
        }

        std::vector<ValidationCheckResult> implValidate(const OpenSim::Model&) const override
        {
            return {
                ValidationCheckResult{"this is an identity warp (i.e. it ignores warping this offset frame altogether)", ValidationCheckState::Warning},
            };
        }
    };

    // a `StationWarpingStrategy` that uses point correspondences and Thin-Plate Spline (TPS) warping to
    // warp the `location` property of the `OpenSim::Station`
    class ThinPlateSplineStationWarpingStrategy final : public StationWarpingStrategy {
        OpenSim_DECLARE_CONCRETE_OBJECT(ThinPlateSplineStationWarpingStrategy, StationWarpingStrategy)
        // TODO: point sources
    private:
        std::unique_ptr<ComponentWarpingStrategy> implClone() const override
        {
            return std::make_unique<ThinPlateSplineStationWarpingStrategy>(*this);
        }

        std::vector<ValidationCheckResult> implValidate(const OpenSim::Model&) const override
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

        std::unique_ptr<IComponentWarper> implCreateWarper(const OpenSim::Model&, const OpenSim::Component&) override
        {
            return std::make_unique<IdentityComponentWarper>();
        }
    };

    // a `StationWarpingStrategy` that always produces an error
    //
    // usually used by configuration writers as a fallback to indicate "if you matched this far
    // then it's an error"
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

        std::unique_ptr<IComponentWarper> implCreateWarper(const OpenSim::Model&, const OpenSim::Component&) override
        {
            return std::make_unique<ExceptionThrowingComponentWarper>("ProduceErrorStationWarpingStrategy: TODO: configuration-customizable error message");
        }

        std::vector<ValidationCheckResult> implValidate(const OpenSim::Model&) const override
        {
            return {
                ValidationCheckResult{"this warping strategy always produces an error (TODO: configuration-customizable error message)", ValidationCheckState::Error},
            };
        }
    };

    // a `StationWarpingStrategy` that leaves the `OpenSim::Station` untouched
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

        std::unique_ptr<IComponentWarper> implCreateWarper(const OpenSim::Model&, const OpenSim::Component&) override
        {
            return std::make_unique<IdentityComponentWarper>();
        }

        std::vector<ValidationCheckResult> implValidate(const OpenSim::Model&) const override
        {
            return {
                ValidationCheckResult{"this is an identity warp (i.e. it ignores warping this station altogether)", ValidationCheckState::Warning},
            };
        }
    };

    // a `StationWarpingStrategy` that uses point correspondences and Thin-Plate Spline (TPS) warping to
    // warp the `location` property of the `OpenSim::Station`
    class ThinPlateSplineMeshWarpingStrategy final : public MeshWarpingStrategy {
        OpenSim_DECLARE_CONCRETE_OBJECT(ThinPlateSplineMeshWarpingStrategy, MeshWarpingStrategy)
            // TODO: point sources
    private:
        std::unique_ptr<ComponentWarpingStrategy> implClone() const override
        {
            return std::make_unique<ThinPlateSplineMeshWarpingStrategy>(*this);
        }

        std::vector<ValidationCheckResult> implValidate(const OpenSim::Model&) const override
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

        std::unique_ptr<IComponentWarper> implCreateWarper(const OpenSim::Model&, const OpenSim::Component&) override
        {
            return std::make_unique<IdentityComponentWarper>();
        }
    };

    class IdentityMeshWarpingStrategy final : public MeshWarpingStrategy {
        OpenSim_DECLARE_CONCRETE_OBJECT(IdentityMeshWarpingStrategy, MeshWarpingStrategy)
    private:
        std::unique_ptr<ComponentWarpingStrategy> implClone() const final
        {
            return std::make_unique<IdentityMeshWarpingStrategy>(*this);
        }

        std::vector<WarpDetail> implWarpDetails() const final
        {
            return {};
        }

        std::unique_ptr<IComponentWarper> implCreateWarper(const OpenSim::Model&, const OpenSim::Component&) final
        {
            return std::make_unique<IdentityComponentWarper>();
        }
    };

    class ProduceErrorMeshWarpingStrategy final : public MeshWarpingStrategy {
        OpenSim_DECLARE_CONCRETE_OBJECT(ProduceErrorMeshWarpingStrategy, MeshWarpingStrategy)
    private:
        std::unique_ptr<ComponentWarpingStrategy> implClone() const final
        {
            return std::make_unique<ProduceErrorMeshWarpingStrategy>(*this);
        }

        std::vector<WarpDetail> implWarpDetails() const final
        {
            return {};
        }

        std::unique_ptr<IComponentWarper> implCreateWarper(const OpenSim::Model&, const OpenSim::Component&) final
        {
            return std::make_unique<ExceptionThrowingComponentWarper>("ProduceErrorMeshWarpingStrategy: TODO: configuration-customizable error message");
        }
    };

    // a configuration object that associatively stores a sequence of
    // `ComponentWarpingStrategy`s that can be associatively matched
    // to `OpenSim::Component`s (presumably, from an `OpenSim::Model`)
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
